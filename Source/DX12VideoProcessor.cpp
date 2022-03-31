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

#include "stdafx.h"
#include <uuids.h>
#include <mfapi.h> // for MR_BUFFER_SERVICE
#include <Mferror.h>
#include <Mfidl.h>
#include <dwmapi.h>
#include <xutility>
#include "Helper.h"
#include "Times.h"
#include "resource.h"
#include "VideoRenderer.h"
#include "../Include/Version.h"

#include "../Include/ID3DVideoMemoryConfiguration.h"
#include "Shaders.h"
#include "Utils/CPUInfo.h"
#include "d3dcompiler.h"
#include <iostream>
#include <fstream>
#include "dxgi1_6.h"

#include "d3d12util/BufferManager.h"

#include "d3d12util/TextRenderer.h"
#include "d3d12util/EsramAllocator.h"
#include "d3d12util/math/Common.h"


#include "../external/minhook/include/MinHook.h"

#include "DX12VideoProcessor.h"
#pragma comment( lib, "d3d12.lib" )
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")
bool g_bPresent12 = false;
bool bCreateSwapChain12 = false;

typedef BOOL(WINAPI* pSetWindowPos)(
	_In_ HWND hWnd,
	_In_opt_ HWND hWndInsertAfter,
	_In_ int X,
	_In_ int Y,
	_In_ int cx,
	_In_ int cy,
	_In_ UINT uFlags);

pSetWindowPos pOrigSetWindowPosDX12 = nullptr;
static BOOL WINAPI pNewSetWindowPosDX12(
	_In_ HWND hWnd,
	_In_opt_ HWND hWndInsertAfter,
	_In_ int X,
	_In_ int Y,
	_In_ int cx,
	_In_ int cy,
	_In_ UINT uFlags)
{
	if (g_bPresent12) {
		DLog(L"call SetWindowPos() function during Present()");
		uFlags |= SWP_ASYNCWINDOWPOS;
	}
	return pOrigSetWindowPosDX12(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

typedef LONG(WINAPI* pSetWindowLongA)(
	_In_ HWND hWnd,
	_In_ int nIndex,
	_In_ LONG dwNewLong);

pSetWindowLongA pOrigSetWindowLongADX12 = nullptr;
static LONG WINAPI pNewSetWindowLongADX12(
	_In_ HWND hWnd,
	_In_ int nIndex,
	_In_ LONG dwNewLong)
{
	if (bCreateSwapChain12) {
		DLog(L"Blocking call SetWindowLongA() function during create fullscreen swap chain");
		return 0L;
	}
	return pOrigSetWindowLongADX12(hWnd, nIndex, dwNewLong);
}

template <typename T>
inline bool HookFunc(T** ppSystemFunction, PVOID pHookFunction)
{
	return MH_CreateHook(*ppSystemFunction, pHookFunction, reinterpret_cast<LPVOID*>(ppSystemFunction)) == MH_OK;
}
using namespace D3D12Engine;

CDX12VideoProcessor::CDX12VideoProcessor(CMpcVideoRenderer* pFilter, const Settings_t& config, HRESULT& hr)
  : CVideoProcessor(pFilter)
{
	m_bShowStats = config.bShowStats;
	m_iResizeStats = config.iResizeStats;
	m_iTexFormat = config.iTexFormat;
	m_VPFormats = config.VPFmts;
	m_bDeintDouble = config.bDeintDouble;
	m_bVPScaling = config.bVPScaling;
	m_iChromaScaling = config.iChromaScaling;
	m_iUpscaling = config.iUpscaling;
	m_iDownscaling = config.iDownscaling;
	m_bInterpolateAt50pct = config.bInterpolateAt50pct;
	m_bUseDither = config.bUseDither;
	m_iSwapEffect = config.iSwapEffect;
	m_bVBlankBeforePresent = config.bVBlankBeforePresent;
	m_bHdrPassthrough = config.bHdrPassthrough;
	m_iHdrToggleDisplay = config.iHdrToggleDisplay;
	m_bConvertToSdr = config.bConvertToSdr;

	m_nCurrentAdapter = -1;
	m_pDisplayMode = &m_DisplayMode;

	hr = CreateDXGIFactory1(IID_IDXGIFactory1, (void**)&m_pDXGIFactory1);
	if (FAILED(hr)) {
		DLog(L"CDX12VideoProcessor::CDX12VideoProcessor() : CreateDXGIFactory1() failed with error {}", HR2Str(hr));
		return;
	}

	// set default ProcAmp ranges and values
	SetDefaultDXVA2ProcAmpRanges(m_DXVA2ProcAmpRanges);
	SetDefaultDXVA2ProcAmpValues(m_DXVA2ProcAmpValues);

	pOrigSetWindowPosDX12 = SetWindowPos;
	auto ret = HookFunc(&pOrigSetWindowPosDX12, pNewSetWindowPosDX12);
	DLogIf(!ret, L"CDX12VideoProcessor::CDX12VideoProcessor() : hook for SetWindowPos() fail");

	pOrigSetWindowLongADX12 = SetWindowLongA;
	ret = HookFunc(&pOrigSetWindowLongADX12, pNewSetWindowLongADX12);
	DLogIf(!ret, L"CDX12VideoProcessor::CDX12VideoProcessor() : hook for SetWindowLongA() fail");

	MH_EnableHook(MH_ALL_HOOKS);

	CComPtr<IDXGIAdapter> pDXGIAdapter;
	for (UINT adapter = 0; m_pDXGIFactory1->EnumAdapters(adapter, &pDXGIAdapter) != DXGI_ERROR_NOT_FOUND; ++adapter) {
		CComPtr<IDXGIOutput> pDXGIOutput;
		for (UINT output = 0; pDXGIAdapter->EnumOutputs(output, &pDXGIOutput) != DXGI_ERROR_NOT_FOUND; ++output) {
			DXGI_OUTPUT_DESC desc{};
			if (SUCCEEDED(pDXGIOutput->GetDesc(&desc))) {
				DisplayConfig_t displayConfig = {};
				if (GetDisplayConfig(desc.DeviceName, displayConfig)) {
					m_hdrModeStartState[desc.DeviceName] = displayConfig.advancedColor.advancedColorEnabled;
					DXGI_ADAPTER_DESC dxgiAdapterDesc = {};
					hr = pDXGIAdapter->GetDesc(&dxgiAdapterDesc);
					if (SUCCEEDED(hr)) {
						m_VendorId = dxgiAdapterDesc.VendorId;
						m_strAdapterDescription = fmt::format(L"{} ({:04X}:{:04X})", dxgiAdapterDesc.Description, dxgiAdapterDesc.VendorId, dxgiAdapterDesc.DeviceId);
						DLog(L"Graphics DXGI adapter: {}", m_strAdapterDescription);
					}
				}
			}

			pDXGIOutput.Release();
		}
		
		pDXGIAdapter.Release();
	}
	for (int xx = 0; xx < 3; xx++)
	{
		SwapChainBuffer[xx] = nullptr;

	}
}

CDX12VideoProcessor::~CDX12VideoProcessor()
{
	D3D12Engine::g_CommandManager.IdleGPU();
	CommandContext::DestroyAllContexts();
	D3D12Engine::g_CommandManager.Shutdown();
	PSO::DestroyAll();
	RootSignature::DestroyAll();
	DescriptorAllocator::DestroyAll();
	ImageScaling::FreeImageScaling();
	/*Need this to have correct heap if we dont entirely close the module*/
	g_DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	g_DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	g_DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	g_DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] = DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	
	D3D12Engine::DestroyCommonState();
	D3D12Engine::DestroyRenderingBuffers();
	m_pPlaneResource[0].DestroyBuffer();
	m_pPlaneResource[1].DestroyBuffer();
	m_pResizeResource.DestroyBuffer();
	m_pVideoOutputResource.DestroyBuffer();
	TextRenderer::Shutdown();
	GeometryRenderer::Shutdown();
	ReleaseSwapChain();
	
	
	g_hWnd = nullptr;
	m_pD3DDebug.Release();
	m_pD3DDebug1.Release();
	m_pDXGIAdapter.Release();
	m_pDXGIFactory2.Release();
	m_pDXGIFactory1.Release();
	m_pDXGIOutput.Release();
	ReleaseDevice();
	MH_RemoveHook(SetWindowPos);
	MH_RemoveHook(SetWindowLongA);
}



void CDX12VideoProcessor::GetHardwareAdapter(
	IDXGIFactory1* pFactory,
	IDXGIAdapter1** ppAdapter,
	bool requestHighPerformanceAdapter)
{
	*ppAdapter = nullptr;

	CComPtr<IDXGIAdapter1> adapter;

	CComPtr<IDXGIFactory6> factory6;
	if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
	{
		for (
			UINT adapterIndex = 0;
			SUCCEEDED(factory6->EnumAdapterByGpuPreference(
				adapterIndex,
				requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
				IID_PPV_ARGS(&adapter)));
			++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);
			
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				m_nCurrentAdapter = adapterIndex;
				break;
			}
		}
	}

	if (adapter == nullptr)
	{
		for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	*ppAdapter = adapter.Detach();
}

HRESULT CDX12VideoProcessor::Init(const HWND hwnd, bool* pChangeDevice)
{

	if (!hwnd)
		return S_OK;

	if (m_hWnd != hwnd)
		m_hWnd = hwnd;
	D3D12Engine::g_hWnd = hwnd;

	if (!D3D12Engine::g_Device)
		return S_OK;
	
	if (D3D12Engine::g_CommandManager.GetGraphicsQueue().IsReady())
		return S_OK;
	UpdateStatsStatic();
	D3D12Engine::g_CommandManager.Create(D3D12Engine::g_Device);
	D3D12Engine::InitializeCommonState();
	D3D12Engine::InitializeRenderingBuffers(1280,528);
	TextRenderer::Initialize();
	GeometryRenderer::Initialize();
	
	ImageScaling::Initialize(m_SwapChainFmt);
	
	SetShaderConvertColorParams();

	return S_OK;
	
}

HRESULT CDX12VideoProcessor::CopySample(IMediaSample* pSample)
{
	CheckPointer(m_pDXGISwapChain1, E_FAIL);

	uint64_t tick = GetPreciseTick();

	// Get frame type
  // no interlace is progressive
	m_SampleFormat = D3D12_VIDEO_FIELD_TYPE_NONE;
	m_bDoubleFrames = false;
	if (m_bInterlaced) {
		if (CComQIPtr<IMediaSample2> pMS2 = pSample) {
			AM_SAMPLE2_PROPERTIES props;
			if (SUCCEEDED(pMS2->GetProperties(sizeof(props), (BYTE*)&props))) {
				if ((props.dwTypeSpecificFlags & AM_VIDEO_FLAG_WEAVE) == 0) {
					if (props.dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD1FIRST) {
						m_SampleFormat = D3D12_VIDEO_FIELD_TYPE_INTERLACED_TOP_FIELD_FIRST; // Top-field first
					}
					else {
						m_SampleFormat = D3D12_VIDEO_FIELD_TYPE_INTERLACED_BOTTOM_FIELD_FIRST; // Bottom-field first
					}
					m_bDoubleFrames = m_bDeintDouble;
				}
			}
		}
	}

	HRESULT hr = S_OK;
	m_FieldDrawn = 0;
	bool updateStats = false;

	m_hdr10 = {};
	if (m_bHdrPassthrough && SourceIsHDR()) {
		if (CComQIPtr<IMediaSideData> pMediaSideData = pSample) {
			MediaSideDataHDR* hdr = nullptr;
			size_t size = 0;
			hr = pMediaSideData->GetSideData(IID_MediaSideDataHDR, (const BYTE**)&hdr, &size);
			if (SUCCEEDED(hr) && size == sizeof(MediaSideDataHDR)) {
				m_hdr10.bValid = true;

				m_hdr10.hdr10.RedPrimary[0] = static_cast<UINT16>(hdr->display_primaries_x[2] * 50000.0);
				m_hdr10.hdr10.RedPrimary[1] = static_cast<UINT16>(hdr->display_primaries_y[2] * 50000.0);
				m_hdr10.hdr10.GreenPrimary[0] = static_cast<UINT16>(hdr->display_primaries_x[0] * 50000.0);
				m_hdr10.hdr10.GreenPrimary[1] = static_cast<UINT16>(hdr->display_primaries_y[0] * 50000.0);
				m_hdr10.hdr10.BluePrimary[0] = static_cast<UINT16>(hdr->display_primaries_x[1] * 50000.0);
				m_hdr10.hdr10.BluePrimary[1] = static_cast<UINT16>(hdr->display_primaries_y[1] * 50000.0);
				m_hdr10.hdr10.WhitePoint[0] = static_cast<UINT16>(hdr->white_point_x * 50000.0);
				m_hdr10.hdr10.WhitePoint[1] = static_cast<UINT16>(hdr->white_point_y * 50000.0);

				m_hdr10.hdr10.MaxMasteringLuminance = static_cast<UINT>(hdr->max_display_mastering_luminance * 10000.0);
				m_hdr10.hdr10.MinMasteringLuminance = static_cast<UINT>(hdr->min_display_mastering_luminance * 10000.0);
			}

			MediaSideDataHDRContentLightLevel* hdrCLL = nullptr;
			size = 0;
			hr = pMediaSideData->GetSideData(IID_MediaSideDataHDRContentLightLevel, (const BYTE**)&hdrCLL, &size);
			if (SUCCEEDED(hr) && size == sizeof(MediaSideDataHDRContentLightLevel)) {
				m_hdr10.hdr10.MaxContentLightLevel = hdrCLL->MaxCLL;
				m_hdr10.hdr10.MaxFrameAverageLightLevel = hdrCLL->MaxFALL;
			}
		}
	}

	if (CComQIPtr<IMediaSampleD3D12> pMSD3D12 = pSample)
	{
		if (m_iSrcFromGPU != 12)
		{
			m_iSrcFromGPU = 12;
			updateStats = true;
		}

		CComQIPtr<ID3D12Resource> pD3D12Resource;
		int index = 0;
		hr = pMSD3D12->GetD3D12Texture(&pD3D12Resource, &index);
		if (FAILED(hr)) {
			DLog(L"CDX12VideoProcessor::CopySample() : GetD3D11Texture() failed with error {}", HR2Str(hr));
			return hr;
		}

		D3D12_RESOURCE_DESC desc;
		desc = pD3D12Resource->GetDesc();



		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layoutplane[2];
		UINT64 pitch_plane[2];
		UINT rows_plane[2];
		UINT64 RequiredSize;
		m_renderRect.IntersectRect(m_videoRect, m_windowRect);
		D3D12Engine::g_Device->GetCopyableFootprints(&desc,
			0, 2, 0, layoutplane, rows_plane, pitch_plane, &RequiredSize);

		//we can't use the format from the footprint its always a typeless format which can't be handled by the copy
		if (desc.Format != DXGI_FORMAT_P010 && m_pPlaneResource[0].GetWidth() == 0)// DXGI_FORMAT_NV12
		{
			m_pPlaneResource[0].Create(L"Scaling Resource", layoutplane[0].Footprint.Width, layoutplane[0].Footprint.Height, 1, DXGI_FORMAT_R8_UNORM);
			m_pPlaneResource[1].Create(L"Scaling Resource", layoutplane[1].Footprint.Width, layoutplane[1].Footprint.Height, 1, DXGI_FORMAT_R8G8_UNORM);
			m_srcDXGIFormat = DXGI_FORMAT_NV12;
		}
		else if (m_pPlaneResource[0].GetWidth() == 0)
		{
			m_pPlaneResource[0].Create(L"Scaling Resource", layoutplane[0].Footprint.Width, layoutplane[0].Footprint.Height, 1, DXGI_FORMAT_R16_UNORM);
			m_pPlaneResource[1].Create(L"Scaling Resource", layoutplane[1].Footprint.Width, layoutplane[1].Footprint.Height, 1, DXGI_FORMAT_R16G16_UNORM);
			m_srcDXGIFormat = DXGI_FORMAT_P010;
		}

		//create the resize resource needed to wait to have a video source
		if (m_pResizeResource.GetWidth() == 0)
		{
			m_pResizeResource.Create(L"Resize Scaling Resource", desc.Width, desc.Height, 1, m_SwapChainFmt);
			m_pVideoOutputResource.Create(L"Resize Scaling Resource Final", desc.Width, desc.Height, 1, m_SwapChainFmt);
		}

		GraphicsContext& pVideoContext = GraphicsContext::Begin(L"Render Video");


		pVideoContext.TransitionResource(m_pPlaneResource[0], D3D12_RESOURCE_STATE_COPY_DEST);
		pVideoContext.TransitionResource(m_pPlaneResource[1], D3D12_RESOURCE_STATE_COPY_DEST, true);
		//need to flush if we dont want an error on the resource state


		D3D12_TEXTURE_COPY_LOCATION dst;
		D3D12_TEXTURE_COPY_LOCATION src;
		for (int i = 0; i < 2; i++) {
			dst = CD3DX12_TEXTURE_COPY_LOCATION(m_pPlaneResource[i].GetResource());
			src = CD3DX12_TEXTURE_COPY_LOCATION(pD3D12Resource, i);
			pVideoContext.GetCommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
		}
		pVideoContext.SetViewportAndScissor(m_videoRect.left, m_videoRect.top, m_videoRect.Width(), m_videoRect.Height());
		ImageScaling::ColorAjust(pVideoContext, m_pResizeResource, m_pPlaneResource[0], m_pPlaneResource[1], m_pBufferVar);
		pVideoContext.Finish();
	}
	else if (CComQIPtr<IMFGetService> pService = pSample)
	{
		if (m_iSrcFromGPU != 9)
		{
			m_iSrcFromGPU = 9;
			updateStats = true;
		}

		CComPtr<IDirect3DSurface9> pSurface9;
		if (SUCCEEDED(pService->GetService(MR_BUFFER_SERVICE, IID_PPV_ARGS(&pSurface9)))) {
			D3DSURFACE_DESC desc = {};
			hr = pSurface9->GetDesc(&desc);
			if (FAILED(hr) || desc.Format != m_srcDXVA2Format) {
				return E_UNEXPECTED;
			}

			if (desc.Width != m_srcWidth || desc.Height != m_srcHeight)
			{
				
				hr = InitializeTexVP(m_srcParams, desc.Width, desc.Height);

				if (FAILED(hr))
					return hr;
				UpdateFrameProperties();
				updateStats = true;
			}

			D3DLOCKED_RECT lr_src;
			hr = pSurface9->LockRect(&lr_src, nullptr, D3DLOCK_READONLY); // may be slow
			if (S_OK == hr) 
			{
				BYTE* srcData = (BYTE*)lr_src.pBits;
				D3D11_MAPPED_SUBRESOURCE mappedResource = {};
#ifdef TODO
				if (m_TexSrcVideo.pTexture2)
				{
					// for Windows 7
					hr = m_pDeviceContext->Map(m_TexSrcVideo.pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
					if (SUCCEEDED(hr)) {
						m_pCopyGpuFn(m_srcHeight, (BYTE*)mappedResource.pData, mappedResource.RowPitch, srcData, lr_src.Pitch);
						m_pDeviceContext->Unmap(m_TexSrcVideo.pTexture, 0);

						hr = m_pDeviceContext->Map(m_TexSrcVideo.pTexture2, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
						if (SUCCEEDED(hr)) {
							srcData += lr_src.Pitch * m_srcHeight;
							m_pCopyGpuFn(m_srcHeight / 2, (BYTE*)mappedResource.pData, mappedResource.RowPitch, srcData, lr_src.Pitch);
							m_pDeviceContext->Unmap(m_TexSrcVideo.pTexture2, 0);
						}
					}
				}
				else
				{
					hr = m_pDeviceContext->Map(m_TexSrcVideo.pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
					if (SUCCEEDED(hr)) {
						m_pCopyGpuFn(m_srcLines, (BYTE*)mappedResource.pData, mappedResource.RowPitch, srcData, lr_src.Pitch);
						m_pDeviceContext->Unmap(m_TexSrcVideo.pTexture, 0);
					}
				}
#endif
			
				

				pSurface9->UnlockRect();

				
			}
		}
	}
	else
	{
		if (m_iSrcFromGPU != 0) {
			m_iSrcFromGPU = 0;
			updateStats = true;
		}

		BYTE* data = nullptr;
		const long size = pSample->GetActualDataLength();
		if (size > 0 && S_OK == pSample->GetPointer(&data)) {
			// do not use UpdateSubresource for D3D11 VP here
			// because it can cause green screens and freezes on some configurations
#ifdef TODO
			hr = MemCopyToTexSrcVideo(data, m_srcPitch);
#endif
			
		}
	}

	return hr;
}


HRESULT CDX12VideoProcessor::ProcessSample(IMediaSample* pSample)
{
	HRESULT hr = S_OK;
	DXGI_SWAP_CHAIN_DESC desc2;
	REFERENCE_TIME rtStart, rtEnd;

	if (m_pFilter->m_filterState == State_Stopped)
		assert(0);
	
	if (!m_pDXGISwapChain1)
		Reset();
	m_pDXGISwapChain1->GetDesc(&desc2);
	if (desc2.OutputWindow != m_hWnd)
		Reset();

	const long size = pSample->GetActualDataLength();
	
	if (FAILED(pSample->GetTime(&rtStart, &rtEnd)))
	{
		rtStart = m_pFilter->m_FrameStats.GeTimestamp();
	}
	const REFERENCE_TIME rtFrameDur = m_pFilter->m_FrameStats.GetAverageFrameDuration();
	rtEnd = rtStart + rtFrameDur;

	m_rtStart = rtStart;
	CRefTime rtClock(rtStart);

	
	hr = CopySample(pSample);

	if (FAILED(hr))
	{
		m_RenderStats.failed++;
		return hr;
	}

	// always Render(1) a frame after CopySample()
	hr = Render(1);

	m_pFilter->m_DrawStats.Add(GetPreciseTick());
	if (m_pFilter->m_filterState == State_Running) {
		m_pFilter->StreamTime(rtClock);
	}

	m_RenderStats.syncoffset = rtClock - rtStart;

	int so = (int)std::clamp(m_RenderStats.syncoffset, -UNITS, UNITS);
#if SYNC_OFFSET_EX
	m_SyncDevs.Add(so - m_Syncs.Last());
#endif
	m_Syncs.Add(so);

	return hr;
	
	
#if 0
	if (m_pFilter->m_filterState == State_Paused)
		return S_OK;
	D3D12_RESOURCE_DESC desc = {};
	CComQIPtr<ID3D12Resource> pD3D12Resource;
	if (CComQIPtr<IMediaSampleD3D12> pMSD3D12 = pSample)
	{
		int index = 0;
		hr = pMSD3D12->GetD3D12Texture(&pD3D12Resource, &index);

		desc = pD3D12Resource->GetDesc();
	}
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT layoutplane[2];
	UINT64 pitch_plane[2];
	UINT rows_plane[2];
	UINT64 RequiredSize;
	m_renderRect.IntersectRect(m_videoRect, m_windowRect);
	D3D12Engine::g_Device->GetCopyableFootprints(&desc,
		0, 2, 0, layoutplane, rows_plane, pitch_plane, &RequiredSize);
	EsramAllocator esram;
	
	//we can't use the format from the footprint its always a typeless format which can't be handled by the copy
	if (desc.Format != DXGI_FORMAT_P010 && m_pPlaneResource[0].GetWidth() == 0)// DXGI_FORMAT_NV12
	{
		m_pPlaneResource[0].Create(L"Scaling Resource", layoutplane[0].Footprint.Width, layoutplane[0].Footprint.Height, 1, DXGI_FORMAT_R8_UNORM);
		m_pPlaneResource[1].Create(L"Scaling Resource", layoutplane[1].Footprint.Width, layoutplane[1].Footprint.Height, 1, DXGI_FORMAT_R8G8_UNORM);
	}
	else if (m_pPlaneResource[0].GetWidth() == 0)
	{
		m_pPlaneResource[0].Create(L"Scaling Resource", layoutplane[0].Footprint.Width, layoutplane[0].Footprint.Height, 1, DXGI_FORMAT_R16_UNORM);
		m_pPlaneResource[1].Create(L"Scaling Resource", layoutplane[1].Footprint.Width, layoutplane[1].Footprint.Height, 1, DXGI_FORMAT_R16G16_UNORM);
	}
	//create the resize resource needed to wait to have a video source
	if (m_pResizeResource.GetWidth() == 0)
	{
		m_pResizeResource.Create(L"Resize Scaling Resource", desc.Width, desc.Height,1, m_SwapChainFmt);
	}

	GraphicsContext& pVideoContext = GraphicsContext::Begin(L"Render Video");


	pVideoContext.TransitionResource(m_pPlaneResource[0], D3D12_RESOURCE_STATE_COPY_DEST);
	//pVideoContext.TransitionResource(m_pPlaneResource[1], D3D12_RESOURCE_STATE_COPY_DEST,true);
	//need to flush if we dont want an error on the resource state


	D3D12_TEXTURE_COPY_LOCATION dst;
	D3D12_TEXTURE_COPY_LOCATION src;
	for (int i = 0; i < 2; i++) {
		dst = CD3DX12_TEXTURE_COPY_LOCATION(m_pPlaneResource[i].GetResource());
		src = CD3DX12_TEXTURE_COPY_LOCATION(pD3D12Resource, i);
		pVideoContext.GetCommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	}

	pVideoContext.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);

	pVideoContext.SetRenderTarget(SwapChainBufferColor[p_CurrentBuffer].GetRTV());

	pVideoContext.ClearColor(SwapChainBufferColor[p_CurrentBuffer]);

	
	
	pVideoContext.SetViewportAndScissor(m_videoRect.left, m_videoRect.top, m_videoRect.Width(), m_videoRect.Height());
	ImageScaling::ColorAjust(pVideoContext, m_pResizeResource, m_pPlaneResource[0], m_pPlaneResource[1], m_pBufferVar);
	pVideoContext.SetViewport(m_videoRect.left, m_videoRect.top, m_videoRect.Width(), m_videoRect.Height());
	/*draw the text*/
	if (m_bShowStats)
	{
		DrawStats(pVideoContext, 10, 10, m_windowRect.Width(), m_windowRect.Height());
	}

	
	ImageScaling::Upscale(pVideoContext, SwapChainBufferColor[p_CurrentBuffer], m_pResizeResource, (ImageScaling::eScalingFilter)m_iUpscaling, m_videoRect);

	pVideoContext.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
	/*Render the overlay*/
	
	/*draw the overlay over the image*/
	if (m_bShowStats)
		ImageScaling::PreparePresentSDR(pVideoContext, SwapChainBufferColor[p_CurrentBuffer], m_pResizeResource, m_videoRect);

	pVideoContext.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_PRESENT);
	pVideoContext.Finish();
	DXGI_PRESENT_PARAMETERS presentParams = { 0 };

	m_pDXGISwapChain4->Present1(1, 0, &presentParams);
	p_CurrentBuffer = (p_CurrentBuffer + 1) % 3;
	if (m_pFilter->m_filterState == State_Running) {
		m_pFilter->StreamTime(rtClock);
	}

	return S_OK;
#endif
}

void CDX12VideoProcessor::UpdateQuad()
{
	return;
	//result = D3D_SetupQuadData(o, &quad->generic, output, pVertexDataBegin, triangleVertices, orientation);
//vertex buffer data pVertexDataBegin dst_data pData
//index buffer data triangleVertices
/*create index buffer and quad vertex buffer*/
	
	VideoQuadVertex vertex_data[4];
	WORD triangleVertices[6];

	D3D12_RANGE readRange = { 0 };
	HRESULT hr = m_pVertexBuffer->Map(0, &readRange, (void**)&vertex_data);
	if (FAILED(hr))
		assert(0);
	hr = m_pIndexBuffer->Map(0, &readRange, (void**)&triangleVertices);
	if (FAILED(hr))
		assert(0);

	//vertex_data = (VideoQuadVertex*)malloc(sizeof(VideoQuadVertex[4]));
	//triangleVertices = (WORD*)malloc(sizeof(triangleVertices[6]));
	unsigned int src_width = m_srcRect.Width();
	unsigned int src_height = m_srcRect.Height();
	float MidX, MidY;

	float top, bottom, left, right;
	/* find the middle of the visible part of the texture, it will be a 0,0
	 * the rest of the visible area must correspond to -1,1 */

	 /* left/top aligned */
	MidY = (m_videoRect.top + m_videoRect.bottom) / 2.f;
	MidX = (m_videoRect.left + m_videoRect.right) / 2.f;
	top = MidY / (MidY - m_videoRect.top);
	bottom = -(src_height - MidY) / (m_videoRect.bottom - MidY);
	left = -MidX / (MidX - m_videoRect.left);
	right = (src_width - MidX) / (m_videoRect.right - MidX);



	const float vertices_coords[4][2] = {
			{ left,  bottom },
			{ right, bottom },
			{ right, top    },
			{ left,  top    },
	};
	int vertex_order[4];
	vertex_order[0] = 0;
	vertex_order[1] = 1;
	vertex_order[2] = 2;
	vertex_order[3] = 3;


	for (int i = 0; i < 4; ++i) {
		vertex_data[i].position.x = vertices_coords[vertex_order[i]][0];
		vertex_data[i].position.y = vertices_coords[vertex_order[i]][1];
	}

	// bottom left
	vertex_data[0].position.z = 0.0f;
	vertex_data[0].texture.u = 0.0f;
	vertex_data[0].texture.v = 1.0f;

	// bottom right
	vertex_data[1].position.z = 0.0f;
	vertex_data[1].texture.u = 1.0f;
	vertex_data[1].texture.v = 1.0f;

	// top right
	vertex_data[2].position.z = 0.0f;
	vertex_data[2].texture.u = 1.0f;
	vertex_data[2].texture.v = 0.0f;

	// top left
	vertex_data[3].position.z = 0.0f;
	vertex_data[3].texture.u = 0.0f;
	vertex_data[3].texture.v = 0.0f;

	/* Make sure surfaces are facing the right way */

	triangleVertices[0] = (WORD)3;
	triangleVertices[1] = (WORD)1;
	triangleVertices[2] = (WORD)0;

	triangleVertices[3] = (WORD)2;
	triangleVertices[4] = (WORD)1;
	triangleVertices[5] = (WORD)3;
	m_pVertexBuffer->Unmap(0, NULL);
	m_pIndexBuffer->Unmap(0, NULL);
	resetquad = false;
	//buffer need to be 16byte aligned which is too small for this vertice
	///void* stackMem = _malloca((sizeof(WORD) + 1) * 16);
	//WORD* stackptr = Math::AlignUp((WORD*)stackMem, 16);
	//stackMem = (void*)triangleVertices;

	//_freea(stackMem);

	
}
void CDX12VideoProcessor::SetupQuad()
{

	
	//VideoQuadVertex = stride
	//vertexcount = 4 on rectangular
	
	m_pVertexBuffer.Create(L"QuadVertex", sizeof(VideoQuadVertex) * 4);//in the good state too start
	m_pVertexBufferView.BufferLocation = m_pVertexBuffer.GetGpuVirtualAddress();
	m_pVertexBufferView.SizeInBytes = m_pVertexBuffer.GetBufferSize();
	m_pVertexBufferView.StrideInBytes = sizeof(VideoQuadVertex);
	//indexcount = 6

	m_pIndexBuffer.Create(L"IndexBuffer", sizeof(WORD)*6);
	m_pIndexBufferView.BufferLocation = m_pVertexBuffer.GetGpuVirtualAddress();
	m_pIndexBufferView.SizeInBytes = m_pVertexBuffer.GetBufferSize();
	m_pIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	
	
	m_pVertexHeapDesc.NumDescriptors = 258;
	m_pVertexHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	m_pVertexHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	m_pVertexHeapDesc.NodeMask = 0;
	//m_pViewpointShaderConstant;
	//m_pPixelShaderConstants;
	HRESULT hr = D3D12Engine::g_Device->CreateDescriptorHeap(&m_pVertexHeapDesc, IID_PPV_ARGS(&m_pVertexHeap));


	m_pPixelShaderConstants.Create(L"Viewpoint Shader Constant", sizeof(*m_sShaderConstants));
	D3D12_RANGE readRange = { 0 }; // no reading

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = m_pPixelShaderConstants.GetGpuVirtualAddress();
	cbvDesc.SizeInBytes = m_pPixelShaderConstants.GetBufferSize();

	D3D12Engine::g_Device->CreateConstantBufferView(&cbvDesc, m_pVertexHeap->GetCPUDescriptorHandleForHeapStart());

	//its ok to leave it on map
	m_pPixelShaderConstants->Map(0, &readRange, (void**)&m_sShaderConstants);

	
	UpdateQuad();
	resetquad = false;
}

HRESULT CDX12VideoProcessor::SetDevice(ID3D12Device* pDevice, const bool bDecoderDevice)
{
	D3D12Engine::g_Device = pDevice;
	Init(m_hWnd, false);
	return S_OK;
}

BOOL CDX12VideoProcessor::VerifyMediaType(const CMediaType* pmt)
{
	const auto& FmtParams = GetFmtConvParams(pmt);
	if (FmtParams.VP11Format == DXGI_FORMAT_UNKNOWN && FmtParams.DX11Format == DXGI_FORMAT_UNKNOWN) {
		return FALSE;
	}

	const BITMAPINFOHEADER* pBIH = GetBIHfromVIHs(pmt);
	if (!pBIH) {
		return FALSE;
	}

	if (pBIH->biWidth <= 0 || !pBIH->biHeight || (!pBIH->biSizeImage && pBIH->biCompression != BI_RGB)) {
		return FALSE;
	}

	if (FmtParams.Subsampling == 420 && ((pBIH->biWidth & 1) || (pBIH->biHeight & 1))) {
		return FALSE;
	}
	if (FmtParams.Subsampling == 422 && (pBIH->biWidth & 1)) {
		return FALSE;
	}

	return TRUE;
}

BOOL CDX12VideoProcessor::InitMediaType(const CMediaType* pmt)
{
	DLog(L"CDX12VideoProcessor::InitMediaType()");

	if (!VerifyMediaType(pmt)) {
		return FALSE;
	}
	auto FmtParams = GetFmtConvParams(pmt);
	m_srcParams = FmtParams;
	bool disableD3D11VP = false;
	switch (FmtParams.cformat) {
	case CF_NV12: disableD3D11VP = !m_VPFormats.bNV12; break;
	case CF_P010:
	case CF_P016: disableD3D11VP = !m_VPFormats.bP01x;  break;
	case CF_YUY2: disableD3D11VP = !m_VPFormats.bYUY2;  break;
	default:      disableD3D11VP = !m_VPFormats.bOther; break;
	}
	

	const BITMAPINFOHEADER* pBIH = nullptr;
	m_decExFmt.value = 0;

	if (pmt->formattype == FORMAT_VideoInfo2) {
		const VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)pmt->pbFormat;
		pBIH = &vih2->bmiHeader;
		m_srcRect = vih2->rcSource;
		m_srcAspectRatioX = vih2->dwPictAspectRatioX;
		m_srcAspectRatioY = vih2->dwPictAspectRatioY;
		if (FmtParams.CSType == CS_YUV && (vih2->dwControlFlags & (AMCONTROL_USED | AMCONTROL_COLORINFO_PRESENT))) {
			m_decExFmt.value = vih2->dwControlFlags;
			m_decExFmt.SampleFormat = AMCONTROL_USED | AMCONTROL_COLORINFO_PRESENT; // ignore other flags
		}
		m_bInterlaced = (vih2->dwInterlaceFlags & AMINTERLACE_IsInterlaced);
		m_rtAvgTimePerFrame = vih2->AvgTimePerFrame;
	}
	else if (pmt->formattype == FORMAT_VideoInfo) {
		const VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->pbFormat;
		pBIH = &vih->bmiHeader;
		m_srcRect = vih->rcSource;
		m_srcAspectRatioX = 0;
		m_srcAspectRatioY = 0;
		m_bInterlaced = 0;
		m_rtAvgTimePerFrame = vih->AvgTimePerFrame;
	}
	else {
		return FALSE;
	}

	m_pFilter->m_FrameStats.SetStartFrameDuration(m_rtAvgTimePerFrame);

	UINT biWidth = pBIH->biWidth;
	UINT biHeight = labs(pBIH->biHeight);
	UINT biSizeImage = pBIH->biSizeImage;
	if (pBIH->biSizeImage == 0 && pBIH->biCompression == BI_RGB) { // biSizeImage may be zero for BI_RGB bitmaps
		biSizeImage = biWidth * biHeight * pBIH->biBitCount / 8;
	}

	m_srcLines = biHeight * FmtParams.PitchCoeff / 2;
	m_srcPitch = biWidth * FmtParams.Packsize;
	switch (FmtParams.cformat) {
	case CF_Y8:
	case CF_NV12:
	case CF_RGB24:
	case CF_BGR48:
		m_srcPitch = ALIGN(m_srcPitch, 4);
		break;
	}
	if (pBIH->biCompression == BI_RGB && pBIH->biHeight > 0) {
		m_srcPitch = -m_srcPitch;
	}

	UINT origW = biWidth;
	UINT origH = biHeight;
	if (pmt->FormatLength() == 112 + sizeof(VR_Extradata)) {
		const VR_Extradata* vrextra = reinterpret_cast<VR_Extradata*>(pmt->pbFormat + 112);
		if (vrextra->QueryWidth == pBIH->biWidth && vrextra->QueryHeight == pBIH->biHeight && vrextra->Compression == pBIH->biCompression) {
			origW = vrextra->FrameWidth;
			origH = abs(vrextra->FrameHeight);
		}
	}

	if (m_srcRect.IsRectNull()) {
		m_srcRect.SetRect(0, 0, origW, origH);
	}
	m_srcRectWidth = m_srcRect.Width();
	m_srcRectHeight = m_srcRect.Height();

	m_srcExFmt = SpecifyExtendedFormat(m_decExFmt, FmtParams, m_srcRectWidth, m_srcRectHeight);

	const auto frm_gcd = std::gcd(m_srcRectWidth, m_srcRectHeight);
	const auto srcFrameARX = m_srcRectWidth / frm_gcd;
	const auto srcFrameARY = m_srcRectHeight / frm_gcd;

	if (!m_srcAspectRatioX || !m_srcAspectRatioY) {
		m_srcAspectRatioX = srcFrameARX;
		m_srcAspectRatioY = srcFrameARY;
		m_srcAnamorphic = false;
	}
	else {
		const auto ar_gcd = std::gcd(m_srcAspectRatioX, m_srcAspectRatioY);
		m_srcAspectRatioX /= ar_gcd;
		m_srcAspectRatioY /= ar_gcd;
		m_srcAnamorphic = (srcFrameARX != m_srcAspectRatioX || srcFrameARY != m_srcAspectRatioY);
	}
#if 0 
	UpdateUpscalingShaders();
	UpdateDownscalingShaders();

	m_pPSCorrection.Release();
	m_pPSConvertColor.Release();
	m_PSConvColorData.bEnable = false;

	UpdateTexParams(FmtParams.CDepth);
#endif
	if (m_bHdrAllowSwitchDisplay && m_srcVideoTransferFunction != m_srcExFmt.VideoTransferFunction) {
		auto ret = HandleHDRToggle();
		if (!ret && (m_bHdrPassthrough && m_bHdrPassthroughSupport && SourceIsHDR() && !m_pDXGISwapChain4)) {
			ret = true;
		}
		if (ret) {
			ReleaseSwapChain();
			Init(m_hWnd);
		}
	}

	if (Preferred10BitOutput() && m_SwapChainFmt == DXGI_FORMAT_B8G8R8A8_UNORM) {
		ReleaseSwapChain();
		Init(m_hWnd);
	}


	m_srcVideoTransferFunction = m_srcExFmt.VideoTransferFunction;
	m_srcWidth = origW;
	m_srcHeight = origH;
	m_srcDXGIFormat = FmtParams.VP11Format;
	m_iSrcFromGPU = 12;/* this will add D3D12 in the stats*/
	//we dont use the video processor yet but at least set the src height and width

#ifdef TODO
	// D3D11 Video Processor
	if (FmtParams.VP11Format != DXGI_FORMAT_UNKNOWN && !(m_VendorId == PCIV_NVIDIA && FmtParams.CSType == CS_RGB)) {
		// D3D11 VP does not work correctly if RGB32 with odd frame width (source or target) on Nvidia adapters

		if (S_OK == InitializeD3D11VP(FmtParams, origW, origH)) {
			bool bTransFunc22 = m_srcExFmt.VideoTransferFunction == DXVA2_VideoTransFunc_22
				|| m_srcExFmt.VideoTransferFunction == DXVA2_VideoTransFunc_709
				|| m_srcExFmt.VideoTransferFunction == DXVA2_VideoTransFunc_240M;

			if (m_srcExFmt.VideoTransferFunction == VIDEOTRANSFUNC_2084 && !(m_bHdrPassthroughSupport && m_bHdrPassthrough) && m_bConvertToSdr) {
				EXECUTE_ASSERT(S_OK == CreatePShaderFromResource(&m_pPSCorrection, IDF_PSH11_CONVERT_PQ_TO_SDR));
				m_strCorrection = L"PQ to SDR";
			}
			else if (m_srcExFmt.VideoTransferFunction == VIDEOTRANSFUNC_HLG) {
				if (m_bHdrPassthroughSupport && m_bHdrPassthrough) {
					EXECUTE_ASSERT(S_OK == CreatePShaderFromResource(&m_pPSCorrection, IDF_PSH11_CONVERT_HLG_TO_PQ));
					m_strCorrection = L"HLG to PQ";
				}
				else if (m_bConvertToSdr) {
					EXECUTE_ASSERT(S_OK == CreatePShaderFromResource(&m_pPSCorrection, IDF_PSH11_CONVERT_HLG_TO_SDR));
					m_strCorrection = L"HLG to SDR";
				}
				else if (m_srcExFmt.VideoPrimaries == VIDEOPRIMARIES_BT2020) {
					// HLG compatible with SDR
					EXECUTE_ASSERT(S_OK == CreatePShaderFromResource(&m_pPSCorrection, IDF_PSH11_FIX_BT2020));
					m_strCorrection = L"Fix BT.2020";
				}
			}
			else if (m_srcExFmt.VideoTransferMatrix == VIDEOTRANSFERMATRIX_YCgCo) {
				EXECUTE_ASSERT(S_OK == CreatePShaderFromResource(&m_pPSCorrection, IDF_PSH11_FIX_YCGCO));
				m_strCorrection = L"Fix YCoCg";
			}
			else if (bTransFunc22 && m_srcExFmt.VideoPrimaries == VIDEOPRIMARIES_BT2020) {
				EXECUTE_ASSERT(S_OK == CreatePShaderFromResource(&m_pPSCorrection, IDF_PSH11_FIX_BT2020));
				m_strCorrection = L"Fix BT.2020";
			}

			DLogIf(m_pPSCorrection, L"CDX12VideoProcessor::InitMediaType() m_pPSCorrection created");

			m_pFilter->m_inputMT = *pmt;
			UpdateTexures();
			UpdatePostScaleTexures();
			UpdateStatsStatic();
			SetCallbackDevice();

			return TRUE;
		}

		ReleaseVP();
	}

	// Tex Video Processor
	if (FmtParams.DX11Format != DXGI_FORMAT_UNKNOWN && S_OK == InitializeTexVP(FmtParams, origW, origH)) {
		m_pFilter->m_inputMT = *pmt;
		SetShaderConvertColorParams();
		UpdateTexures();
		UpdatePostScaleTexures();
		UpdateStatsStatic();
		SetCallbackDevice();

		return TRUE;
	}

	return FALSE;
#endif
	return TRUE;
}

HRESULT CDX12VideoProcessor::InitializeTexVP(const FmtConvParams_t& params, const UINT width, const UINT height)
{
#ifdef TODO
	const auto& srcDXGIFormat = params.DX11Format;

	DLog(L"CDX12VideoProcessor::InitializeTexVP() started with input surface: {}, {} x {}", DXGIFormatToString(srcDXGIFormat), width, height);

	UINT texW = (params.cformat == CF_YUY2) ? width / 2 : width;

	HRESULT hr = m_TexSrcVideo.CreateEx(m_pDevice, srcDXGIFormat, params.pDX11Planes, texW, height, Tex2D_DynamicShaderWrite);
	if (FAILED(hr)) {
		DLog(L"CDX12VideoProcessor::InitializeTexVP() : m_TexSrcVideo.CreateEx() failed with error {}", HR2Str(hr));
		return hr;
	}

	m_srcWidth = width;
	m_srcHeight = height;
	m_srcParams = params;
	m_srcDXGIFormat = srcDXGIFormat;
	m_srcDXVA2Format = params.DXVA2Format;
	m_pConvertFn = GetCopyFunction(params);

	// set default ProcAmp ranges
	SetDefaultDXVA2ProcAmpRanges(m_DXVA2ProcAmpRanges);

	HRESULT hr2 = UpdateConvertColorShader();
	if (FAILED(hr2)) {
		ASSERT(0);
		UINT resid = 0;
		if (params.cformat == CF_YUY2) {
			resid = IDF_PSH11_CONVERT_YUY2;
		}
		else if (params.pDX11Planes) {
			if (params.pDX11Planes->FmtPlane3) {
				if (params.cformat == CF_YV12 || params.cformat == CF_YV16 || params.cformat == CF_YV24) {
					resid = IDF_PSH11_CONVERT_PLANAR_YV;
				}
				else {
					resid = IDF_PSH11_CONVERT_PLANAR;
				}
			}
			else {
				resid = IDF_PSH11_CONVERT_BIPLANAR;
			}
		}
		else {
			resid = IDF_PSH11_CONVERT_COLOR;
		}
		EXECUTE_ASSERT(S_OK == CreatePShaderFromResource(&m_pPSConvertColor, resid));
	}

	SAFE_RELEASE(m_PSConvColorData.pVertexBuffer);
	EXECUTE_ASSERT(S_OK == CreateVertexBuffer(m_pDevice, &m_PSConvColorData.pVertexBuffer, m_srcWidth, m_srcHeight, m_srcRect, 0, false));

	DLog(L"CDX12VideoProcessor::InitializeTexVP() completed successfully");

#endif
	return S_OK;
}

void CDX12VideoProcessor::Configure(const Settings_t& config)
{
	bool reloadPipeline = false;
	bool changeWindow = false;
	bool changeDevice = false;
	bool changeVP = false;
	bool changeHDR = false;
	bool changeTextures = false;
	bool changeConvertShader = false;
	bool changeUpscalingShader = false;
	bool changeDowndcalingShader = false;
	bool changeNumTextures = false;
	bool changeResizeStats = false;

	// settings that do not require preparation
	m_bShowStats = config.bShowStats;
	m_bDeintDouble = config.bDeintDouble;
	m_bInterpolateAt50pct = config.bInterpolateAt50pct;
	m_bVBlankBeforePresent = config.bVBlankBeforePresent;

	// checking what needs to be changed

	if (config.iResizeStats != m_iResizeStats) {
		m_iResizeStats = config.iResizeStats;
		changeResizeStats = true;
	}

	if (config.iTexFormat != m_iTexFormat) {
		m_iTexFormat = config.iTexFormat;
		changeTextures = true;
	}

	if (m_srcParams.cformat == CF_NV12) {
		changeVP = config.VPFmts.bNV12 != m_VPFormats.bNV12;
	}
	else if (m_srcParams.cformat == CF_P010 || m_srcParams.cformat == CF_P016) {
		changeVP = config.VPFmts.bP01x != m_VPFormats.bP01x;
	}
	else if (m_srcParams.cformat == CF_YUY2) {
		changeVP = config.VPFmts.bYUY2 != m_VPFormats.bYUY2;
	}
	else {
		changeVP = config.VPFmts.bOther != m_VPFormats.bOther;
	}
	m_VPFormats = config.VPFmts;

	if (config.bVPScaling != m_bVPScaling) {
		m_bVPScaling = config.bVPScaling;
		changeTextures = true;
		changeVP = true; // temporary solution
	}
#ifdef TODO
	if (config.iChromaScaling != m_iChromaScaling) {
		m_iChromaScaling = config.iChromaScaling;
		changeConvertShader = m_PSConvColorData.bEnable && (m_srcParams.Subsampling == 420 || m_srcParams.Subsampling == 422);
	}
#endif
	if (config.iUpscaling != m_iUpscaling) {
		m_iUpscaling = config.iUpscaling;
		changeUpscalingShader = true;
	}
	if (config.iDownscaling != m_iDownscaling) {
		m_iDownscaling = config.iDownscaling;
		changeDowndcalingShader = true;
	}

	if (config.bUseDither != m_bUseDither) {
		m_bUseDither = config.bUseDither;
		changeNumTextures = m_InternalTexFmt != DXGI_FORMAT_B8G8R8A8_UNORM;
	}

	if (config.iSwapEffect != m_iSwapEffect) {
		m_iSwapEffect = config.iSwapEffect;
		changeWindow = !m_pFilter->m_bIsFullscreen;
	}

	if (config.bHdrPassthrough != m_bHdrPassthrough) {
		m_bHdrPassthrough = config.bHdrPassthrough;
		changeHDR = true;
	}

	if (config.iHdrToggleDisplay != m_iHdrToggleDisplay) {
		if (config.iHdrToggleDisplay == HDRTD_Off || m_iHdrToggleDisplay == HDRTD_Off) {
			changeHDR = true;
		}
		m_iHdrToggleDisplay = config.iHdrToggleDisplay;
	}
#ifdef TODO
	if (config.bConvertToSdr != m_bConvertToSdr) {
		m_bConvertToSdr = config.bConvertToSdr;
		if (SourceIsHDR()) {
			if (m_D3D11VP.IsReady()) {
				changeNumTextures = true;
				changeVP = true; // temporary solution
			}
			else {
				changeConvertShader = true;
			}
		}
	}
#endif
	// apply new settings

	if (changeWindow) {
		ReleaseSwapChain();
		EXECUTE_ASSERT(S_OK == m_pFilter->Init(true));

		if (changeHDR && (SourceIsHDR()) || m_iHdrToggleDisplay) {
			m_srcVideoTransferFunction = 0;
			InitMediaType(&m_pFilter->m_inputMT);
		}
		return;
	}

	if (changeHDR) {
		if (SourceIsHDR() || m_iHdrToggleDisplay) {
			if (m_iSwapEffect == SWAPEFFECT_Discard) {
				ReleaseSwapChain();
				m_pFilter->Init(true);
			}

			m_srcVideoTransferFunction = 0;
			InitMediaType(&m_pFilter->m_inputMT);

			return;
		}
	}

	if (changeVP) {
		InitMediaType(&m_pFilter->m_inputMT);
		m_pFilter->m_bValidBuffer = false;
		return; // need some test
	}
#ifdef TODO
	if (changeTextures) {
		UpdateTexParams(m_srcParams.CDepth);
		if (m_D3D11VP.IsReady()) {
			// update m_D3D11OutputFmt
			EXECUTE_ASSERT(S_OK == InitializeD3D11VP(m_srcParams, m_srcWidth, m_srcHeight));
		}
		UpdateTexures();
		UpdatePostScaleTexures();
	}
#endif
	if (changeConvertShader) {
		reloadPipeline = true;
		//UpdateConvertColorShader();
	}

	if (changeUpscalingShader) {
		reloadPipeline = true;//UpdateUpscalingShaders();
	}
	if (changeDowndcalingShader) {
		reloadPipeline = true;//UpdateDownscalingShaders();
	}

	if (changeNumTextures) {
		reloadPipeline = true;// UpdatePostScaleTexures();
	}

	if (changeResizeStats) {
		SetGraphSize();
	}

	UpdateStatsStatic();
	return;
}

void CDX12VideoProcessor::SetRotation(int value)
{
}

void CDX12VideoProcessor::Flush()
{
	m_rtStart = 0;
	D3D12Engine::g_CommandManager.IdleGPU();
	return;
}

void CDX12VideoProcessor::ClearPreScaleShaders()
{
}

void CDX12VideoProcessor::ClearPostScaleShaders()
{
}

HRESULT CDX12VideoProcessor::AddPreScaleShader(const std::wstring& name, const std::string& srcCode)
{
	return E_NOTIMPL;
}

HRESULT CDX12VideoProcessor::AddPostScaleShader(const std::wstring& name, const std::string& srcCode)
{
	return E_NOTIMPL;
}

bool CDX12VideoProcessor::Initialized()
{
  return false;
}

void CDX12VideoProcessor::ReleaseDevice()
{
	DLog(L"CDX12VideoProcessor::ReleaseDevice()");
	
	m_pVertexBuffer.Destroy();
	m_pIndexBuffer.Destroy();
	m_pViewpointShaderConstant.Destroy();
	m_pPixelShaderConstants.Destroy();
	m_pVertexHeap.Release();
	g_Device = nullptr;
	
	
}

void CDX12VideoProcessor::ReleaseSwapChain()
{
	
	for (uint32_t i = 0; i < 3; ++i)
	{
		SwapChainBufferColor[i].Destroy();
	}
	if (m_pDXGISwapChain1) {
		m_pDXGISwapChain1->SetFullscreenState(FALSE, nullptr);
	}
	m_pDXGIOutput.Release();
	m_pDXGISwapChain4.Release();
	m_pDXGISwapChain1.Release();

}

bool CDX12VideoProcessor::HandleHDRToggle()
{
	return false;
}

void CDX12VideoProcessor::SetGraphSize()
{
	if (!m_windowRect.IsRectEmpty()) {
		SIZE rtSize = m_windowRect.Size();

		CalcStatsFont();
		if (S_OK == TextRenderer::LoadFont(L"Consolas", m_StatsFontH, 0)) {
			SIZE charSize = TextRenderer::GetMaxCharMetric();
			m_StatsRect.right = m_StatsRect.left + 60 * charSize.cx + 5 + 3;
			m_StatsRect.bottom = m_StatsRect.top + 18 * charSize.cy + 5 + 3;
		}
		m_StatsBackground.graphrect = m_StatsRect;
		m_StatsBackground.graphcolor = D3DCOLOR_ARGB(80, 0, 0, 0);
		CalcGraphParams();
		m_Underlay.graphrect = m_GraphRect;
		m_Underlay.graphcolor = D3DCOLOR_ARGB(80, 0, 0, 0);
		//TODO
		//m_Lines.ClearPoints(rtSize);
		POINT points[2];
		const int linestep = 20 * m_Yscale;
		GraphLine lne;
		m_Lines.clear();
		for (int y = m_GraphRect.top + (m_Yaxis - m_GraphRect.top) % (linestep); y < m_GraphRect.bottom; y += linestep) {
			lne.linertsize = rtSize;
			lne.linepoints[0] = { m_GraphRect.left,  y };
			lne.linepoints[1] = { m_GraphRect.right, y };
			lne.linecolor = (y == m_Yaxis) ? D3DCOLOR_XRGB(150, 150, 255) : D3DCOLOR_XRGB(100, 100, 255);
			lne.linesize = 2;
			m_Lines.push_back(lne);
			//TODO
			//m_Lines.AddPoints(points, std::size(points), (y == m_Yaxis) ? D3DCOLOR_XRGB(150, 150, 255) : D3DCOLOR_XRGB(100, 100, 255));
		}
		//TODO
		//m_Lines.UpdateVertexBuffer();
	}
}

BOOL CDX12VideoProcessor::GetAlignmentSize(const CMediaType& mt, SIZE& Size)
{
	//TODO
	if (InitMediaType(&mt)) {
		const auto& FmtParams = GetFmtConvParams(&mt);

		if (FmtParams.cformat == CF_RGB24) {
			Size.cx = ALIGN(Size.cx, 4);
		}
		else if (FmtParams.cformat == CF_RGB48 || FmtParams.cformat == CF_BGR48) {
			Size.cx = ALIGN(Size.cx, 2);
		}
		else if (FmtParams.cformat == CF_BGRA64 || FmtParams.cformat == CF_B64A) {
			// nothing
		}
		else 
		{
			//if (!m_TexSrcVideo.pTexture) {
			//	return FALSE;
			//}

			UINT RowPitch = 0;
			D3D11_MAPPED_SUBRESOURCE mappedResource = {};
			//if (SUCCEEDED(m_pDeviceContext->Map(m_TexSrcVideo.pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
			//	RowPitch = mappedResource.RowPitch;
			//	m_pDeviceContext->Unmap(m_TexSrcVideo.pTexture, 0);
			//}

			//if (!RowPitch) {
			//	return FALSE;
			//}
			
			Size.cx = RowPitch / FmtParams.Packsize;
			//temporary
			Size.cx = m_srcRect.right - m_srcRect.left;
		}

		if (FmtParams.cformat == CF_RGB24 || FmtParams.cformat == CF_XRGB32 || FmtParams.cformat == CF_ARGB32) {
			Size.cy = -abs(Size.cy); // only for biCompression == BI_RGB
		}
		else {
			Size.cy = abs(Size.cy);
		}

		return TRUE;

	}

	return FALSE;
}

void CDX12VideoProcessor::SetShaderConvertColorParams()
{
	mp_csp_params csp_params;
	set_colorspace(m_srcExFmt, csp_params.color);
	csp_params.brightness = DXVA2FixedToFloat(m_DXVA2ProcAmpValues.Brightness) / 255;
	csp_params.contrast = DXVA2FixedToFloat(m_DXVA2ProcAmpValues.Contrast);
	csp_params.hue = DXVA2FixedToFloat(m_DXVA2ProcAmpValues.Hue) / 180 * acos(-1);
	csp_params.saturation = DXVA2FixedToFloat(m_DXVA2ProcAmpValues.Saturation);
	csp_params.gray = m_srcParams.CSType == CS_GRAY;

	csp_params.input_bits = csp_params.texture_bits = m_srcParams.CDepth;

	m_PSConvColorData =
		m_srcParams.CSType == CS_YUV ||
		m_srcParams.cformat == CF_GBRP8 || m_srcParams.cformat == CF_GBRP16 ||
		csp_params.gray ||
		fabs(csp_params.brightness) > 1e-4f || fabs(csp_params.contrast - 1.0f) > 1e-4f;

	mp_cmat cmatrix;
	mp_get_csp_matrix(&csp_params, &cmatrix);
	m_pBufferVar = {
		{cmatrix.m[0][0], cmatrix.m[0][1], cmatrix.m[0][2], 0},
		{cmatrix.m[1][0], cmatrix.m[1][1], cmatrix.m[1][2], 0},
		{cmatrix.m[2][0], cmatrix.m[2][1], cmatrix.m[2][2], 0},
		{cmatrix.c[0],    cmatrix.c[1],    cmatrix.c[2],    0},
	};

	if (m_srcParams.cformat == CF_Y410 || m_srcParams.cformat == CF_Y416) {
		std::swap(m_pBufferVar.cm_r.x, m_pBufferVar.cm_r.y);
		std::swap(m_pBufferVar.cm_g.x, m_pBufferVar.cm_g.y);
		std::swap(m_pBufferVar.cm_b.x, m_pBufferVar.cm_b.y);
	}
	else if (m_srcParams.cformat == CF_GBRP8 || m_srcParams.cformat == CF_GBRP16) {
		std::swap(m_pBufferVar.cm_r.x, m_pBufferVar.cm_r.y); std::swap(m_pBufferVar.cm_r.y, m_pBufferVar.cm_r.z);
		std::swap(m_pBufferVar.cm_g.x, m_pBufferVar.cm_g.y); std::swap(m_pBufferVar.cm_g.y, m_pBufferVar.cm_g.z);
		std::swap(m_pBufferVar.cm_b.x, m_pBufferVar.cm_b.y); std::swap(m_pBufferVar.cm_b.y, m_pBufferVar.cm_b.z);
	}
	else if (csp_params.gray) {
		m_pBufferVar.cm_g.x = m_pBufferVar.cm_g.y;
		m_pBufferVar.cm_g.y = 0;
		m_pBufferVar.cm_b.x = m_pBufferVar.cm_b.z;
		m_pBufferVar.cm_b.z = 0;
	}

}



void CDX12VideoProcessor::DrawStats(GraphicsContext& Context, float x, float y, float w, float h)
{
	//Switch to the overlay buffer
	Context.TransitionResource(g_OverlayBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	Context.ClearColor(g_OverlayBuffer);
	Context.SetRenderTarget(g_OverlayBuffer.GetRTV());
	Context.SetViewportAndScissor(0, 0, g_OverlayBuffer.GetWidth(), g_OverlayBuffer.GetHeight());

	GeometryContext BackgroundWindow(Context);
	BackgroundWindow.Begin();
	SIZE rtSize = m_windowRect.Size();
	BackgroundWindow.DrawRectangle(m_StatsRect,rtSize, D3DCOLOR_ARGB(80, 0, 0, 0));
	BackgroundWindow.End();
	
	//L"Consolas", m_StatsFontH, 0

	std::wstring str;
	str.reserve(700);
	str.assign(m_strStatsHeader);
	str.append(m_strStatsDispInfo);
	str += fmt::format(L"\nGraph. Adapter: {}", m_strAdapterDescription);
	wchar_t frametype = 'p';//(m_SampleFormat != D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE) ? 'i' : 'p';
	str += fmt::format(
		L"\nFrame rate    : {:7.3f}{},{:7.3f}",
		m_pFilter->m_FrameStats.GetAverageFps(),
		frametype,
		m_pFilter->m_DrawStats.GetAverageFps()
	);

	str.append(m_strStatsInputFmt);
	str.append(m_strStatsVProc);
	const int dstW = m_videoRect.Width();
	const int dstH = m_videoRect.Height();
	str += fmt::format(L"\nScaling       : {}x{} -> {}x{}", m_srcRectWidth, m_srcRectHeight, dstW, dstH);
	//enum eScalingFilter { kBilinear, kSharpening, kBicubic, kLanczos, kFilterCount };
	if (m_iUpscaling == 0)
		str.append(L" Bilinear");
	else if (m_iUpscaling == 1)
		str.append(L" Sharpening");
	else if (m_iUpscaling == 2)
		str.append(L" Bicubic");
	else if (m_iUpscaling == 3)
		str.append(L" kLanczos");
	str.append(m_strStatsHDR);
	str.append(m_strStatsPresent);
	str += fmt::format(L"\nFrames: {:5}, skipped: {}/{}, failed: {}",
		m_pFilter->m_FrameStats.GetFrames(), m_pFilter->m_DrawStats.m_dropped, m_RenderStats.dropped2, m_RenderStats.failed);
	str += fmt::format(L"\nTimes(ms): Copy{:3}, Paint{:3} [DX9Subs{:3}], Present{:3}",
		m_RenderStats.copyticks * 1000 / GetPreciseTicksPerSecondI(),
		m_RenderStats.paintticks * 1000 / GetPreciseTicksPerSecondI(),
		m_RenderStats.substicks * 1000 / GetPreciseTicksPerSecondI(),
		m_RenderStats.presentticks * 1000 / GetPreciseTicksPerSecondI());
	str += fmt::format(L"\nSync offset   : {:+3} ms", (m_RenderStats.syncoffset + 5000) / 10000);
	
	//load font will return immediatly if already loaded
	TextRenderer::LoadFont(L"Consolas", m_StatsFontH, 0);
	FontContext textRenderer(Context);

	textRenderer.Begin();
	textRenderer.DrawTextW(rtSize, m_StatsTextPoint.x, m_StatsTextPoint.y, m_dwStatsTextColor,str.c_str());

	textRenderer.End();


	GeometryContext SyncGraph(Context);
	
	if (CheckGraphPlacement()) {
		SyncGraph.Begin();
		SyncGraph.DrawRectangle(m_Underlay.graphrect, rtSize, m_Underlay.graphcolor);
		for (std::vector<GraphLine>::iterator it = m_Lines.begin(); it != m_Lines.end(); it++)
			SyncGraph.DrawLine(*it);
		SyncGraph.DrawGFPoints(m_GraphRect.left, m_Xstep, m_Yaxis, m_Yscale,
			m_Syncs.Data(), m_Syncs.OldestIndex(), m_Syncs.Size(), D3DCOLOR_XRGB(100, 200, 100), rtSize);

		SyncGraph.End();
	}

}

HRESULT CDX12VideoProcessor::Render(int field)
{
	CheckPointer(m_pDXGISwapChain1, E_FAIL);
	HRESULT hr = S_OK;
	if (field)
		m_FieldDrawn = field;

	uint64_t tick1 = GetPreciseTick();

	HRESULT hrSubPic = E_FAIL;

	GraphicsContext& pVideoContext = GraphicsContext::Begin(L"Render Video");

	if (m_pFilter->m_pSubCallBack)
	{
		const CRect rSrcPri(CPoint(0, 0), m_windowRect.Size());
		const CRect rDstVid(m_videoRect);
		const auto rtStart = m_pFilter->m_rtStartTime + m_rtStart;

		if (m_bSubPicWasRendered) {
			m_bSubPicWasRendered = false;
			m_pD3DDevEx->ColorFill(m_pSurface9SubPic, nullptr, m_pFilter->m_bSubInvAlpha ? D3DCOLOR_ARGB(0, 0, 0, 0) : D3DCOLOR_ARGB(255, 0, 0, 0));
		}

		if (CComQIPtr<ISubRenderCallback4> pSubCallBack4 = m_pFilter->m_pSubCallBack) {
			hrSubPic = pSubCallBack4->RenderEx3(rtStart, 0, m_rtAvgTimePerFrame, rDstVid, rDstVid, rSrcPri);
		}
		else {
			hrSubPic = m_pFilter->m_pSubCallBack->Render(rtStart, rDstVid.left, rDstVid.top, rDstVid.right, rDstVid.bottom, rSrcPri.Width(), rSrcPri.Height());
		}

		if (S_OK == hrSubPic) {
			m_bSubPicWasRendered = true;

			// flush Direct3D9 for immediate update Direct3D11 texture
			CComPtr<IDirect3DQuery9> pEventQuery;
			m_pD3DDevEx->CreateQuery(D3DQUERYTYPE_EVENT, &pEventQuery);
			if (pEventQuery) {
				pEventQuery->Issue(D3DISSUE_END);
				BOOL Data = FALSE;
				while (S_FALSE == pEventQuery->GetData(&Data, sizeof(Data), D3DGETDATA_FLUSH));
			}
		}
	}

	uint64_t tick2 = GetPreciseTick();

	if (!m_windowRect.IsRectEmpty())
	{
		GraphicsContext& pVideoContext = GraphicsContext::Begin(L"Clear Target");
		pVideoContext.SetViewportAndScissor(m_windowRect.left, m_windowRect.top, m_windowRect.Width(), m_windowRect.Height());
		pVideoContext.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
		pVideoContext.SetRenderTarget(SwapChainBufferColor[p_CurrentBuffer].GetRTV());

		pVideoContext.ClearColor(SwapChainBufferColor[p_CurrentBuffer]);
		pVideoContext.Finish();
	}

	if (!m_renderRect.IsRectEmpty())
	{
		hr = Process(m_srcRect, m_videoRect, m_FieldDrawn == 2);
	}

	if (S_OK == hrSubPic) {
		const CRect rSrcPri(CPoint(0, 0), m_windowRect.Size());

		D3D11_VIEWPORT VP;
		VP.TopLeftX = 0;
		VP.TopLeftY = 0;
		VP.Width = rSrcPri.Width();
		VP.Height = rSrcPri.Height();
		VP.MinDepth = 0.0f;
		VP.MaxDepth = 1.0f;
#ifdef TODO
		hrSubPic = AlphaBltSub(m_pShaderResourceSubPic, pBackBuffer, rSrcPri, VP);
#endif
		ASSERT(S_OK == hrSubPic);
	}

	pVideoContext.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
	pVideoContext.SetRenderTarget(SwapChainBufferColor[p_CurrentBuffer].GetRTV());

	if (m_bShowStats)
		DrawStats(pVideoContext, 10, 10, m_windowRect.Width(), m_windowRect.Height());

	pVideoContext.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_PRESENT);
	
	if (m_bAlphaBitmapEnable)
	{
		D3D12_RESOURCE_DESC desc;
		SwapChainBufferColor[0]->GetDesc();
		D3D11_VIEWPORT VP = {
			m_AlphaBitmapNRectDest.left * desc.Width,
			m_AlphaBitmapNRectDest.top * desc.Height,
			(m_AlphaBitmapNRectDest.right - m_AlphaBitmapNRectDest.left) * desc.Width,
			(m_AlphaBitmapNRectDest.bottom - m_AlphaBitmapNRectDest.top) * desc.Height,
			0.0f,
			1.0f
		};
#ifdef TODO
		hr = AlphaBlt(m_TexAlphaBitmap.pShaderResource, pBackBuffer, m_pAlphaBitmapVertex, &VP, m_pSamplerLinear);
#endif
	}


	m_RenderStats.substicks = tick2 - tick1; // after DrawStats to relate to paintticks
	uint64_t tick3 = GetPreciseTick();
	m_RenderStats.paintticks = tick3 - tick1;
	//TODO do not use m_pDXGISwapChain4 if we are not in hdr
	if (0)//m_pDXGISwapChain4) 
	{
		const DXGI_COLOR_SPACE_TYPE colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
		if (m_currentSwapChainColorSpace != colorSpace) {
			if (m_hdr10.bValid) {
				hr = m_pDXGISwapChain4->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(DXGI_HDR_METADATA_HDR10), &m_hdr10.hdr10);
				DLogIf(FAILED(hr), L"CDX11VideoProcessor::Render() : SetHDRMetaData(hdr) failed with error {}", HR2Str(hr));

				m_lastHdr10 = m_hdr10;
				UpdateStatsStatic();
			}
			else if (m_lastHdr10.bValid) {
				hr = m_pDXGISwapChain4->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(DXGI_HDR_METADATA_HDR10), &m_lastHdr10.hdr10);
				DLogIf(FAILED(hr), L"CDX11VideoProcessor::Render() : SetHDRMetaData(lastHdr) failed with error {}", HR2Str(hr));
			}
			else {
				m_lastHdr10.bValid = true;

				m_lastHdr10.hdr10.RedPrimary[0] = 34000; // Display P3 primaries
				m_lastHdr10.hdr10.RedPrimary[1] = 16000;
				m_lastHdr10.hdr10.GreenPrimary[0] = 13250;
				m_lastHdr10.hdr10.GreenPrimary[1] = 34500;
				m_lastHdr10.hdr10.BluePrimary[0] = 7500;
				m_lastHdr10.hdr10.BluePrimary[1] = 3000;
				m_lastHdr10.hdr10.WhitePoint[0] = 15635;
				m_lastHdr10.hdr10.WhitePoint[1] = 16450;
				m_lastHdr10.hdr10.MaxMasteringLuminance = 1000 * 10000; // 1000 nits
				m_lastHdr10.hdr10.MinMasteringLuminance = 100;          // 0.01 nits
				hr = m_pDXGISwapChain4->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(DXGI_HDR_METADATA_HDR10), &m_lastHdr10.hdr10);
				DLogIf(FAILED(hr), L"CDX11VideoProcessor::Render() : SetHDRMetaData(Display P3 standard) failed with error {}", HR2Str(hr));

				UpdateStatsStatic();
			}

			UINT colorSpaceSupport = 0;
			if (SUCCEEDED(m_pDXGISwapChain4->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport))
				&& (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) == DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) {
				hr = m_pDXGISwapChain4->SetColorSpace1(colorSpace);
				DLogIf(FAILED(hr), L"CDX11VideoProcessor::Render() : SetColorSpace1() failed with error {}", HR2Str(hr));
				if (SUCCEEDED(hr)) {
					m_currentSwapChainColorSpace = colorSpace;
				}
			}
		}
		else if (m_hdr10.bValid) {
			if (memcmp(&m_hdr10.hdr10, &m_lastHdr10.hdr10, sizeof(m_hdr10.hdr10)) != 0) {
				hr = m_pDXGISwapChain4->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(DXGI_HDR_METADATA_HDR10), &m_hdr10.hdr10);
				DLogIf(FAILED(hr), L"CDX11VideoProcessor::Render() : SetHDRMetaData(hdr) failed with error {}", HR2Str(hr));

				m_lastHdr10 = m_hdr10;
				UpdateStatsStatic();
			}
		}
	}

	pVideoContext.Finish();

	if (m_bVBlankBeforePresent && m_pDXGIOutput) {
		hr = m_pDXGIOutput->WaitForVBlank();
		DLogIf(FAILED(hr), L"WaitForVBlank failed with error {}", HR2Str(hr));
	}

	g_bPresent12 = true;
	hr = m_pDXGISwapChain1->Present(1, 0);
	p_CurrentBuffer = (p_CurrentBuffer + 1) % 3;
	g_bPresent12 = false;
	DLogIf(FAILED(hr), L"CDX11VideoProcessor::Render() : Present() failed with error {}", HR2Str(hr));

	m_RenderStats.presentticks = GetPreciseTick() - tick3;
#ifdef TODO
	if (hr == DXGI_ERROR_INVALID_CALL && m_pFilter->m_bIsD3DFullscreen)
		InitSwapChain();
#endif
	return hr;
}

HRESULT CDX12VideoProcessor::Process(const CRect& srcRect, const CRect& dstRect, const bool second)
{
	HRESULT hr = S_OK;
	m_bDitherUsed = false;
	int rotation = m_iRotation;

	CRect rSrc = srcRect;
	Tex2D_t* pInputTexture = nullptr;
#ifdef TODO
	bool bNeedPostProc = m_pPSCorrection || m_pPostScaleShaders.size();
	if (m_PSConvColorData.bEnable) {
		ConvertColorPass(m_TexConvertOutput.pTexture);
		pInputTexture = &m_TexConvertOutput;
		rSrc.SetRect(0, 0, m_TexConvertOutput.desc.Width, m_TexConvertOutput.desc.Height);
	}
	else {
		pInputTexture = &m_TexSrcVideo;
	}
#endif
	GraphicsContext& pVideoContext = GraphicsContext::Begin(L"Render Onto SwapChain");
	if (rSrc != dstRect) 
	{
		ImageScaling::Upscale(pVideoContext, m_pVideoOutputResource, m_pResizeResource, (ImageScaling::eScalingFilter)m_iUpscaling, m_videoRect);
		pVideoContext.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
		ImageScaling::PreparePresentSDR(pVideoContext, SwapChainBufferColor[p_CurrentBuffer], m_pVideoOutputResource, m_videoRect);
	}
	else
	{
		pVideoContext.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
		ImageScaling::PreparePresentSDR(pVideoContext, SwapChainBufferColor[p_CurrentBuffer], m_pResizeResource, m_videoRect);

	}
	pVideoContext.Finish();
	DLogIf(FAILED(hr), L"CDX9VideoProcessor::Process() : failed with error {}", HR2Str(hr));

	return hr;
}

HRESULT CDX12VideoProcessor::FillBlack()
{
    return S_OK;
}

void CDX12VideoProcessor::SetVideoRect(const CRect& videoRect)
{
	m_videoRect = videoRect;
	
		
	m_renderRect.IntersectRect(m_videoRect, m_windowRect);
	
	resetquad = true;
}

HRESULT CDX12VideoProcessor::SetWindowRect(const CRect& windowRect)
{
	
	m_windowRect = windowRect;
	m_renderRect.IntersectRect(m_videoRect, m_windowRect);
	//m_renderRect.left = m_windowRect.Width() - m_renderRect.right;
	const UINT w = m_windowRect.Width();
	const UINT h = m_windowRect.Height();
	if (m_pDXGISwapChain1)
	{
		D3D12Engine::g_CommandManager.IdleGPU();
		for (uint32_t i = 0; i < 3; ++i)
		{
			SwapChainBufferColor[i].Destroy();
		}
		HRESULT hr = m_pDXGISwapChain4->ResizeBuffers(3, w, h, m_SwapChainFmt, 0);
		if (SUCCEEDED(hr))
		{
			
			//SwapChainBufferColor[3]

			CComPtr<ID3D12Resource> DisplayPlane;

			//m_pDXGISwapChain4->GetBuffer(i, MY_IID_PPV_ARGS(&DisplayPlane));
			//SwapChainBufferColor[i].CreateFromSwapChain(L"Primary SwapChain Buffer", DisplayPlane.Detach());
			for (uint32_t i = 0; i < 3; ++i)
			{
				CComPtr<ID3D12Resource> DisplayPlane;
				m_pDXGISwapChain4->GetBuffer(i, MY_IID_PPV_ARGS(&DisplayPlane));
				SwapChainBufferColor[i].CreateFromSwapChain(L"Primary SwapChain Buffer", DisplayPlane.Detach());
			}
			D3D12Engine::g_OverlayBuffer.Create(L"UI Overlay", w, h, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
			D3D12Engine:: g_HorizontalBuffer.Create(L"Bicubic Intermediate", w, h, 1, DXGI_FORMAT_R10G10B10A2_UNORM);
			p_CurrentBuffer = 0;
			D3D12Engine::g_CommandManager.IdleGPU();
		}
	}
	SetGraphSize();
	resetquad = true;
	return S_OK;
}

HRESULT CDX12VideoProcessor::Reset()
{
	for (int i = 0; i < 3; i++)
	{
		SwapChainBufferColor[i].Destroy();
		if(SwapChainBuffer[i])
			SwapChainBuffer[i]->Release();
	}

	m_pDXGISwapChain1.Release();

	CComPtr<IDXGIFactory4> dxgiFactory;
	EXECUTE_ASSERT(S_OK == CreateDXGIFactory2(0, MY_IID_PPV_ARGS(&dxgiFactory)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = m_windowRect.Width();
	swapChainDesc.Height = m_windowRect.Height();
	swapChainDesc.Format = m_SwapChainFmt;// DXGI_FORMAT_R10G10B10A2_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 3;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
	fsSwapChainDesc.Windowed = TRUE;

	
	HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(
		D3D12Engine::g_CommandManager.GetCommandQueue(),
		D3D12Engine::g_hWnd,
		&swapChainDesc,
		&fsSwapChainDesc,
		nullptr,
		&m_pDXGISwapChain1);

	hr = m_pDXGISwapChain1->QueryInterface(MY_IID_PPV_ARGS(&m_pDXGISwapChain4)); 
	m_currentSwapChainColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
	for (UINT i = 0; i < 3; i++)
	{
		hr = m_pDXGISwapChain4->GetBuffer(i, IID_ID3D12Resource, (void**)&SwapChainBuffer[i]);
		SwapChainBufferColor[i].CreateFromSwapChain(L"Primary SwapChain Buffer", SwapChainBuffer[i]);
	}

	hr = m_pDXGISwapChain1->GetContainingOutput(&m_pDXGIOutput);
	dxgiFactory.Release();
	DXGI_SWAP_CHAIN_DESC desc2;
	m_pDXGISwapChain1->GetDesc(&desc2);
	
	if (desc2.OutputWindow != m_hWnd)
		assert(0);
    return E_NOTIMPL;
}

HRESULT CDX12VideoProcessor::GetCurentImage(long* pDIBImage)
{
    return E_NOTIMPL;
}

HRESULT CDX12VideoProcessor::GetDisplayedImage(BYTE** ppDib, unsigned* pSize)
{
  return E_NOTIMPL;
}

HRESULT CDX12VideoProcessor::GetVPInfo(std::wstring& str)
{
  return E_NOTIMPL;
}

void CDX12VideoProcessor::UpdateStatsPresent()
{
}

void CDX12VideoProcessor::UpdateStatsStatic()
{
	if (m_srcParams.cformat) {
		m_strStatsHeader = fmt::format(L"MPC VR {}, Direct3D 12", _CRT_WIDE(VERSION_STR));

		UpdateStatsInputFmt();

		/*m_strStatsVProc.assign(L"\nVideoProcessor: ");
		if (0) {
			m_strStatsVProc += fmt::format(L"D3D11 VP, output to {}", DXGIFormatToString(m_D3D11OutputFmt));
		}
		else {
			m_strStatsVProc.append(L"Shaders");
			if (m_srcParams.Subsampling == 420 || m_srcParams.Subsampling == 422) {
				m_strStatsVProc.append(L", Chroma scaling: ");
				switch (m_iChromaScaling) {
				case CHROMA_Nearest:
					m_strStatsVProc.append(L"Nearest-neighbor");
					break;
				case CHROMA_Bilinear:
					m_strStatsVProc.append(L"Bilinear");
					break;
				case CHROMA_CatmullRom:
					m_strStatsVProc.append(L"Catmull-Rom");
					break;
				}
			}
		}*/
		m_strStatsVProc += fmt::format(L"\nInternalFormat: {}", DXGIFormatToString(m_InternalTexFmt));

		if (SourceIsHDR()) {
			m_strStatsHDR.assign(L"\nHDR processing: ");
			if (m_bHdrPassthroughSupport && m_bHdrPassthrough) {
				m_strStatsHDR.append(L"Passthrough");
				if (m_lastHdr10.bValid) {
					m_strStatsHDR += fmt::format(L", {} nits", m_lastHdr10.hdr10.MaxMasteringLuminance / 10000);
				}
			}
			else if (m_bConvertToSdr) {
				m_strStatsHDR.append(L"Convert to SDR");
			}
			else {
				m_strStatsHDR.append(L"Not used");
			}
		}
		else {
			m_strStatsHDR.clear();
		}

		UpdateStatsPresent();
	}
	else {
		m_strStatsHeader = L"Error";
		m_strStatsVProc.clear();
		m_strStatsInputFmt.clear();
		//m_strStatsPostProc.clear();
		m_strStatsHDR.clear();
		m_strStatsPresent.clear();
	}
}

STDMETHODIMP_(HRESULT __stdcall) CDX12VideoProcessor::SetProcAmpValues(DWORD dwFlags, DXVA2_ProcAmpValues* pValues)
{
  return E_NOTIMPL;
}

STDMETHODIMP_(HRESULT __stdcall) CDX12VideoProcessor::SetAlphaBitmap(const MFVideoAlphaBitmap* pBmpParms)
{
  return E_NOTIMPL;
}

STDMETHODIMP_(HRESULT __stdcall) CDX12VideoProcessor::UpdateAlphaBitmapParameters(const MFVideoAlphaBitmapParams* pBmpParms)
{
  return E_NOTIMPL;
}


void CDX12VideoProcessor::SetCallbackDevice(const bool bChangeDevice/* = false*/)
{
	if ((!m_bCallbackDeviceIsSet || bChangeDevice) && m_pD3DDevEx && m_pFilter->m_pSubCallBack) {
		m_bCallbackDeviceIsSet = SUCCEEDED(m_pFilter->m_pSubCallBack->SetDevice(m_pD3DDevEx));
	}
}