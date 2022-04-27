/*
* (C) 2022 Ti-BEN
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

#pragma once

#include "stdafx.h"
#include "RenderThreadDX11.h"
#include "../Include/ID3DVideoMemoryConfiguration.h"
#include <mfapi.h>
#include <Mfidl.h>
#include "Helper.h"
//#include "DX9VideoProcessor.h"
#include "DX11VideoProcessor.h"
//#include "ImageScaling.h"//todo remove

HRESULT CVideoBufferManager11::Alloc(int size,D3D11_TEXTURE2D_DESC texdesc)
{
	//not sure about the lock here
	//CAutoLock lock(this);
	HRESULT hr = S_OK;
	FreeBuffers();
	for (int i = 0; i < size; i++)
	{
		CVideoBuffer tex = {};
		m_pDevice->CreateTexture2D(&texdesc, nullptr, &TEX11GET(tex.pVideoTexture));
		D3D11_SHADER_RESOURCE_VIEW_DESC shaderDesc;
		shaderDesc.Format = texdesc.Format;
		shaderDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shaderDesc.Texture2D.MostDetailedMip = 0; // = Texture2D desc.MipLevels - 1
		shaderDesc.Texture2D.MipLevels = 1;       // = Texture2D desc.MipLevels

		hr = m_pDevice->CreateShaderResourceView(TEX11GET(tex.pVideoTexture), &shaderDesc, &tex.pShaderResource);


		m_pBufferFreeList.push(tex);
	}
	return hr;
}

CRenderThread11::CRenderThread11(CDX11VideoProcessor* pVideoProcessor)
  : CRenderThread(),
	m_pVideoProcessor(pVideoProcessor)
{
	m_pVideoTexture = {};
	
}

CRenderThread11::~CRenderThread11()
{

}

HRESULT CRenderThread11::CopySample(IMediaSample* pSample)
{
	
	HRESULT hr = S_OK;
	CVideoBuffer buf;
	hr = m_pInputBuffers->GetFreeBuffer(buf);
	if (FAILED(hr))
		return hr;
	//set timing will later be used for rendering subtitles and present
	pSample->GetTime(&buf.rtStart, &buf.rtStop);
	buf.rtDuration = buf.rtStop - buf.rtStart;

	if (CComQIPtr<IMediaSampleD3D11> pMSD3D11 = pSample) {
		CComQIPtr<ID3D11Texture2D> pD3D11Texture2D;
		UINT ArraySlice = 0;
		hr = pMSD3D11->GetD3D11Texture(0, &pD3D11Texture2D, &ArraySlice);
		if (FAILED(hr)) {
			
			return hr;
		}
		D3D11_BOX srcBox = { 0, 0, 0, m_pVideoProcessor->m_srcWidth, m_pVideoProcessor->m_srcHeight, 1 };
		m_pVideoProcessor->m_pDeviceContext->CopySubresourceRegion(m_pVideoTexture.pTexture, 0, 0, 0, 0, pD3D11Texture2D, ArraySlice, &srcBox);
		
		hr = m_pVideoProcessor->ConvertColorPassThread(std::get<CComPtr<ID3D11Texture2D>>(buf.pVideoTexture), m_pVideoTexture);
		//if succeeded put the buffer in ready state if not return it as free

	}
	else {
		BYTE* data = nullptr;
		const long size = pSample->GetActualDataLength();
		if (size > 0 && S_OK == pSample->GetPointer(&data)) {
			// do not use UpdateSubresource for D3D11 VP here
			// because it can cause green screens and freezes on some configurations
			hr = m_pVideoProcessor->MemCopyToTexSrcVideoThread(data, m_pVideoProcessor->m_srcPitch, m_pVideoTexture);
			hr = m_pVideoProcessor->ConvertColorPassThread(std::get<CComPtr<ID3D11Texture2D>>(buf.pVideoTexture), m_pVideoTexture);
		}

	}
	if (SUCCEEDED(hr))
		m_pInputBuffers->SetBufferReady(buf);
	else
		m_pInputBuffers->ReturnFreeBuffer(buf);
	return hr;
	return hr;
}

HRESULT CRenderThread11::ProcessShader(CVideoBuffer& inBuf, CVideoBuffer& outBuf)
{

	
	HRESULT hr = m_pVideoProcessor->ProcessThread(TEX11GET(outBuf.pVideoTexture), TEX11GET(inBuf.pVideoTexture), inBuf.pShaderResource, m_pVideoProcessor->m_srcRect, m_pVideoProcessor->m_videoRect, false);

	return hr;
}

HRESULT CRenderThread11::ProcessSubs(CVideoBuffer& inBuf, CVideoBuffer& outBuf)
{


	HRESULT hr = m_pVideoProcessor->RenderSubs(inBuf, outBuf);

	return hr;
}

HRESULT CRenderThread11::ProcessPresents(CVideoBuffer& inBuf)
{
	HRESULT hr = m_pVideoProcessor->PresentThread(inBuf);

	return hr;
}

HRESULT CRenderThread11::Init(int inputBufferCount, int processBufferCount, int presentBufferCount, FmtConvParams_t param)
{
	
	m_pInputBuffers = nullptr; m_pProcessBuffers = nullptr; m_pPresentBuffers = nullptr;
	if (!m_pInputBuffers)
		m_pInputBuffers = new CVideoBufferManager11(m_pVideoProcessor->m_pDevice);
	if (!m_pProcessBuffers)
		m_pProcessBuffers = new CVideoBufferManager11(m_pVideoProcessor->m_pDevice);
	if (!m_pPresentBuffers)
		m_pPresentBuffers = new CVideoBufferManager11(m_pVideoProcessor->m_pDevice);
	

	D3D11_TEXTURE2D_DESC texdesc = CreateTex2DDesc(m_pVideoProcessor->m_SwapChainFmt, m_pVideoProcessor->m_srcWidth, m_pVideoProcessor->m_srcHeight, Tex2D_DefaultShaderRTarget);
	UINT texW = (param.cformat == CF_YUY2) ? m_pVideoProcessor->m_srcWidth / 2 : m_pVideoProcessor->m_srcWidth;
	m_pVideoTexture.CreateEx(m_pVideoProcessor->m_pDevice, m_pVideoProcessor->m_srcDXGIFormat, param.pDX11Planes, texW, m_pVideoProcessor->m_srcHeight, Tex2D_DynamicShaderWrite);
	HRESULT hrr = S_OK;
	hrr = m_pProcessBuffers->Alloc(inputBufferCount, texdesc);
	hrr = m_pPresentBuffers->Alloc(inputBufferCount, texdesc);
	hrr = m_pInputBuffers->Alloc(inputBufferCount, texdesc);

	
	return hrr;
}

void CRenderThread11::Resize(int size)
{
	
}

void CRenderThread11::ReleaseBuffers()
{
}

void CRenderThread11::Flush()
{
}

