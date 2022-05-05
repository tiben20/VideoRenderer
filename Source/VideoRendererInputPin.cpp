/*
 * (C) 2018-2021 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include <Mferror.h>
#include "VideoRenderer.h"
#include "VideoRendererInputPin.h"
#include "CustomAllocator.h"

//
// CVideoRendererInputPin
//

CVideoRendererInputPin::CVideoRendererInputPin(CBaseRenderer *pRenderer, HRESULT *phr, LPCWSTR Name, CTBD12VideoRenderer* pBaseRenderer)
	: CRendererInputPin(pRenderer, phr, Name)
	, m_pBaseRenderer(pBaseRenderer)
{
}

CVideoRendererInputPin::~CVideoRendererInputPin()
{
	SAFE_DELETE(m_pNewMT);
}

STDMETHODIMP CVideoRendererInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	if (riid == __uuidof(IMFGetService)) {
		return GetInterface((IMFGetService*)this, ppv);
	}
	else if (riid == __uuidof(ID3D12DecoderConfiguration) && m_pBaseRenderer->m_VideoProcessor->Type() == VP_DX12) {
		return GetInterface((ID3D12DecoderConfiguration*)this, ppv);
	}
	else {
		return __super::NonDelegatingQueryInterface(riid, ppv);
	}
}

STDMETHODIMP CVideoRendererInputPin::GetAllocator(IMemAllocator **ppAllocator)
{
	DLog(L"CVideoRendererInputPin::GetAllocator()");

	if (m_bD3D12) {
		// Renderer shouldn't manage allocator for DXVA/D3D11
		return E_NOTIMPL;
	}

	CheckPointer(ppAllocator, E_POINTER);

	if (m_pAllocator) {
		// We already have an allocator, so return that one.
		*ppAllocator = m_pAllocator;
		(*ppAllocator)->AddRef();
		return S_OK;
	}

	// No allocator yet, so propose our custom allocator.
	HRESULT hr = S_OK;
	m_pCustomAllocator = new(std::nothrow) CCustomAllocator(L"Custom allocator", nullptr, this, &hr);
	if (!m_pCustomAllocator) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		SAFE_DELETE(m_pCustomAllocator);
		return hr;
	}

	if (m_pNewMT) {
		m_pCustomAllocator->SetNewMediaType(*m_pNewMT);
		SAFE_DELETE(m_pNewMT);
	}

	// Return the IMemAllocator interface to the caller.
	return m_pCustomAllocator->QueryInterface(IID_IMemAllocator, (void**)ppAllocator);
}

STDMETHODIMP CVideoRendererInputPin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES* pProps)
{
	// 1 buffer required
	ZeroMemory(pProps, sizeof(ALLOCATOR_PROPERTIES));
	pProps->cbBuffer = 1;
	return S_OK;
}

STDMETHODIMP CVideoRendererInputPin::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
{
	DLog(L"CVideoRendererInputPin::ReceiveConnection()");

	CAutoLock cObjectLock(m_pLock);

	if (m_Connected) {
		CMediaType mt(*pmt);

		if (FAILED(CheckMediaType(&mt))) {
			return VFW_E_TYPE_NOT_ACCEPTED;
		}

		ALLOCATOR_PROPERTIES props, actual;

		CComPtr<IMemAllocator> pMemAllocator;
		if (FAILED(GetAllocator(&pMemAllocator))
				|| FAILED(pMemAllocator->Decommit())
				|| FAILED(pMemAllocator->GetProperties(&props))) {
			return FrameInVideoMem()
				? S_OK // hack for Microsoft DTV-DVD Video Decoder
				: E_FAIL;
		}

		CMediaType mtNew(*pmt);
		props.cbBuffer = m_pBaseRenderer->CalcImageSize(mtNew, !FrameInVideoMem());

		if (FAILED(pMemAllocator->SetProperties(&props, &actual))
				|| FAILED(pMemAllocator->Commit())
				|| props.cbBuffer != actual.cbBuffer) {
			return E_FAIL;
		}

		return SetMediaType(&mt) == S_OK // here set mt, not mtNew
			? S_OK
			: VFW_E_TYPE_NOT_ACCEPTED;
	} else {
		m_bD3D12 = false;
	}

	return __super::ReceiveConnection(pConnector, pmt);
}

STDMETHODIMP CVideoRendererInputPin::NewSegment(REFERENCE_TIME startTime, REFERENCE_TIME stopTime, double rate)
{
	CAutoLock cReceiveLock(&m_csReceive);

	m_pBaseRenderer->NewSegment(startTime);
	return CRendererInputPin::NewSegment(startTime, stopTime, rate);
}

STDMETHODIMP CVideoRendererInputPin::BeginFlush()
{
	CAutoLock cReceiveLock(&m_csReceive);
	return CRendererInputPin::BeginFlush();
}

// IMFGetService
STDMETHODIMP CVideoRendererInputPin::GetService(REFGUID guidService, REFIID riid, LPVOID *ppvObject)
{
	return m_pBaseRenderer->GetService(guidService, riid, ppvObject);
}

// ID3D11DecoderConfiguration
STDMETHODIMP CVideoRendererInputPin::ActivateD3D12Decoding(ID3D12Device* pDevice, HANDLE hMutex, UINT nFlags)
{
	HRESULT hr = E_FAIL;
	if (m_pBaseRenderer->m_VideoProcessor->Type() == VP_DX12) {
		if (auto DX12VP = dynamic_cast<CDX12VideoProcessor*>(m_pBaseRenderer->m_VideoProcessor)) {
			hr = DX12VP->SetDevice(pDevice, true);
		}
	}
	m_bD3D12 = (hr == S_OK);
	DLog(L"CVideoRendererInputPin::ActivateD3D12Decoding() : {}", m_bD3D12 ? L"completed successfully" : L"failed");
	return hr;
}

UINT STDMETHODCALLTYPE CVideoRendererInputPin::GetD3D12AdapterIndex()
{
	return m_pBaseRenderer->m_VideoProcessor->GetCurrentAdapter();
}

void CVideoRendererInputPin::SetNewMediaType(const CMediaType& mt)
{
	DLog(L"CVideoRendererInputPin::SetNewMediaType()");

	SAFE_DELETE(m_pNewMT);
	if (m_pCustomAllocator) {
		m_pCustomAllocator->SetNewMediaType(mt);
	} else {
		m_pNewMT = new CMediaType(mt);
	}
}

void CVideoRendererInputPin::ClearNewMediaType()
{
	SAFE_DELETE(m_pNewMT);
	if (m_pCustomAllocator) {
		m_pCustomAllocator->ClearNewMediaType();
	}
}
