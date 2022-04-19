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
#include <mfapi.h>
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
#include "DX12Capabilities.h"
#include "TextRenderer.h"
#include "math/Common.h"
#include "../external/minhook/include/MinHook.h"
#include "Scaler.h"
#include "DX12VideoProcessor.h"
#include "ShadersLoader.h"
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

static const wchar_t* s_ChromaUpscalingName[9] = {
	{L"Nearest-neighbor"  },
	{L"Bilinear"},
	{L"Cubic"       },
	{L"Lanczos"          },
	{L"Spline"          },
	{L"Jinc"          },
	{L"Bilateral"          },
	{L"Reconstruction"          },
	{L"super-xbr"          },
};

static const wchar_t* s_UpscalingDoubling[1] = {
	{L"super-xbr"  },
};

static const wchar_t* s_UpscalingName[9] = {
	{L"Upscaling Bilinear#0"  },
	{L"Upscaling DXVA2#1"},
	{L"Upscaling Cubic#2"       },
	{L"Upscaling Lanczos#3"          },
	{L"Upscaling Spline#4"          },
	{L"Upscaling Jinc#5"          },
	{L"Upscaling Superxbr#6"          },
	{L"Upscaling fxrcnnx#7"          },
	{L"Upscaling superresxbr#8"          },
};

static const wchar_t* s_DownscalingName[7] = {
	{L"Downscaling Nearest-neighbor#0"  },
  {L"Downscaling DXVA2#1"},
	{L"Downscaling Cubic#2"     },
	{L"Downscaling Lanczos#3"      },
	{L"Downscaling Spline#4"          },
	{L"Downscaling Jinc#5"          },
	{L"Downscaling SSIM#6"          },
	
};

using namespace D3D12Engine;

CDX12VideoProcessor::CDX12VideoProcessor(CMpcVideoRenderer* pFilter, const Settings_t& config, HRESULT& hr)
  : CVideoProcessor(pFilter)
{
	
	m_bForceD3D12 = config.D3D12Settings.bForceD3D12;
	m_bShowStats = config.bShowStats;
	m_iResizeStats = config.iResizeStats;
	m_iTexFormat = config.iTexFormat;
	m_VPFormats = config.VPFmts;
	m_bDeintDouble = config.bDeintDouble;
	m_bVPScaling = config.bVPScaling;
	m_bInterpolateAt50pct = config.bInterpolateAt50pct;
	m_bUseDither = config.bUseDither;
	m_iSwapEffect = config.iSwapEffect;
	m_bVBlankBeforePresent = config.bVBlankBeforePresent;
	m_bHdrPassthrough = config.bHdrPassthrough;
	m_iHdrToggleDisplay = config.iHdrToggleDisplay;
	m_bConvertToSdr = config.bConvertToSdr;
	

	m_nCurrentAdapter = -1;
	m_pDisplayMode = &m_DisplayMode;
	if (FAILED(D3D12Engine::InitDXGIFactory()))
		return;
	


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

	
	D3D12Engine::DestroyCommonState();

	D3D12Engine::ReleaseEngine();
	
	TextRenderer::Shutdown();
	GeometryRenderer::Shutdown();
	ReleaseSwapChain();
	m_pAlphaBitmapTexture.Destroy();

	m_pTexturePlane1.Destroy();
	m_pTexturePlane2.Destroy();
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

	if (m_bForceD3D12 && !D3D12Engine::g_Device)
	{
		D3D12Engine::CreateDevice();	
	}
	if (!D3D12Engine::g_Device)
		return S_OK;
	
	if (D3D12Engine::g_CommandManager.GetGraphicsQueue().IsReady())
		return S_OK;

	D3D12Engine::g_CommandManager.Create(D3D12Engine::g_Device);
	//INIT videoprocessor
#ifdef TODO
	m_D3D12VP.InitVideoDevice(D3D12Engine::g_Device);
#endif
	D3D12Engine::InitializeCommonState();
	TextRenderer::Initialize();
	GeometryRenderer::Initialize();
	
	ImageScaling::Initialize(D3D12Engine::GetSwapChainFormat());
	
	SetShaderConvertColorParams(m_srcExFmt, m_srcParams, m_DXVA2ProcAmpValues);
	UpdateStatsStatic();
	IDXGIAdapter* pDXGIAdapter = nullptr;
	const UINT currentAdapter = GetAdapter(hwnd, D3D12Engine::GetDXGIFactory(), &pDXGIAdapter);
	//TODO do this on the engine side
	if (!m_bCallbackDeviceIsSet)
		m_nCurrentAdapter = currentAdapter;

	if (m_nCurrentAdapter == currentAdapter)
	{
		SAFE_RELEASE(pDXGIAdapter);
		if (hwnd) {
			HRESULT hr = InitDX9Device(hwnd, pChangeDevice);
			ASSERT(S_OK == hr);
			if (m_pD3DDevEx)
			{
				// set a special blend mode for alpha channels for ISubRenderCallback rendering
				// this is necessary for the second alpha blending
				m_pD3DDevEx->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
				m_pD3DDevEx->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
				m_pD3DDevEx->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
				m_pD3DDevEx->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);

				SetCallbackDevice(pChangeDevice ? *pChangeDevice : false);
				CreateSubPicSurface();
			}
		}
	}
	m_nCurrentAdapter = currentAdapter;
	return S_OK;
	
}

void CDX12VideoProcessor::CreateSubPicSurface()
{
	m_pSurface9SubPic.Release();
	if (m_pTextureSubPic.GetWidth() > 0)
		m_pTextureSubPic.DestroyBuffer();
	HRESULT hr = S_OK;
	if (m_pD3DDevEx)
	{
		HANDLE sharedHandle = nullptr;
		hr = m_pD3DDevEx->CreateRenderTarget(
			m_d3dpp.BackBufferWidth,
			m_d3dpp.BackBufferHeight,
			D3DFMT_A8R8G8B8,
			D3DMULTISAMPLE_NONE,
			0,
			FALSE,
			&m_pSurface9SubPic,
			&sharedHandle);
		DLogIf(FAILED(hr), L"CDX11VideoProcessor::InitSwapChain() : CreateRenderTarget(Direct3D9) failed with error {}", HR2Str(hr));

		if (m_pSurface9SubPic) {
			//should not happen we would lose every heap pointers
			ID3D12Resource* resource;
			hr = D3D12Engine::g_Device->OpenSharedHandle(sharedHandle, IID_PPV_ARGS(&resource));
			m_pTextureSubPic.CreateShared(L"Shared subpic texture",
				m_d3dpp.BackBufferWidth, m_d3dpp.BackBufferHeight, 1,
				DXGI_FORMAT_B8G8R8A8_UNORM, resource);
			//hr = D3D12Engine::g_Device->OpenSharedHandle(sharedHandle, IID_PPV_ARGS(&m_pTextureSubPic));
			DLogIf(FAILED(hr), L"CDX11VideoProcessor::InitSwapChain() : OpenSharedResource() failed with error {}", HR2Str(hr));
			
		}
		if (m_pTextureSubPic.GetWidth()>0) {
			hr = m_pD3DDevEx->ColorFill(m_pSurface9SubPic, nullptr, D3DCOLOR_ARGB(255, 0, 0, 0));
			hr = m_pD3DDevEx->SetRenderTarget(0, m_pSurface9SubPic);
			DLogIf(FAILED(hr), L"CDX11VideoProcessor::InitSwapChain() : SetRenderTarget(Direct3D9) failed with error {}", HR2Str(hr));
		}

		return;
	
	}


}

HRESULT CDX12VideoProcessor::CopySample(IMediaSample* pSample)
{
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

		//TODO
#ifdef TODO
		m_D3D12VP.Process(pD3D12Resource, nullptr);
#endif

		hr = D3D12Engine::CopySample(pD3D12Resource);
		
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
				assert(0);
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

			hr = MemCopyToTexSrcVideo(data, m_srcPitch, size);

		}
	}
	m_RenderStats.copyticks = GetPreciseTick() - tick;
	return hr;
}

HRESULT CDX12VideoProcessor::MemCopyToTexSrcVideo(const BYTE* srcData, const int srcPitch, size_t bufferlength)
{
	HRESULT hr = S_FALSE;
	
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT layoutplane[2];
	UINT64 pitch_plane[2];
	UINT rows_plane[2];
	UINT64 RequiredSize;
	D3D12_RESOURCE_DESC desc;
	if (m_srcParams.DX11Format == DXGI_FORMAT_NV12 || m_srcParams.DX11Format == DXGI_FORMAT_P010)
	{
		desc = CD3DX12_RESOURCE_DESC::Tex2D(m_srcParams.DX11Format, m_srcRect.Width(), m_srcRect.Height(), 1, 1);
		g_Device->GetCopyableFootprints(&desc, 0, 2, 0, layoutplane, rows_plane, pitch_plane, &RequiredSize);
		layoutplane[0].Footprint.Format = m_srcParams.pDX11Planes->FmtPlane1;
		layoutplane[1].Footprint.Format = m_srcParams.pDX11Planes->FmtPlane2;
	}
	
	size_t firstplane = pitch_plane[0] * m_srcHeight;
	Texture texture1;
	Texture texture2;
	
	if (m_pTexturePlane1.GetWidth()>0)
	{
#if 0
		GraphicsContext& pVideoContext = GraphicsContext::Begin(L"Create texture transit");
		pVideoContext.TransitionResource(m_pTexturePlane1, D3D12_RESOURCE_STATE_COPY_DEST);
		pVideoContext.TransitionResource(m_pTexturePlane2, D3D12_RESOURCE_STATE_COPY_DEST);
		pVideoContext.Finish(true);
		D3D12_SUBRESOURCE_DATA texResource1;
		texResource1.pData = srcData;
		texResource1.RowPitch = srcPitch;
		texResource1.SlicePitch = srcPitch * m_srcHeight;
		CommandContext::InitializeTexture(m_pTexturePlane1, 1, &texResource1);
		D3D12_SUBRESOURCE_DATA texResource2;
		texResource2.pData = srcData + firstplane;
		texResource2.RowPitch = srcPitch;
		texResource2.SlicePitch = srcPitch * layoutplane[1].Footprint.Height;
		CommandContext::InitializeTexture(m_pTexturePlane2, 1, &texResource2);
#endif
		m_pTexturePlane1.Destroy();
		m_pTexturePlane2.Destroy();
		m_pTexturePlane1.Create2D(srcPitch, m_srcWidth, m_srcHeight, m_srcParams.pDX11Planes->FmtPlane1, srcData);
		m_pTexturePlane2.Create2D(srcPitch, layoutplane[1].Footprint.Width, layoutplane[1].Footprint.Height, m_srcParams.pDX11Planes->FmtPlane2, srcData + firstplane);
	}
	else
	{
		m_pTexturePlane1.Create2D(srcPitch, m_srcWidth, m_srcHeight, m_srcParams.pDX11Planes->FmtPlane1, srcData);
		m_pTexturePlane2.Create2D(srcPitch, layoutplane[1].Footprint.Width, layoutplane[1].Footprint.Height, m_srcParams.pDX11Planes->FmtPlane2, srcData + firstplane);
	}
	
	//TODO 1 plane or 3 planes
	
	hr = CopySampleShaderPassSW(m_pTexturePlane1, m_pTexturePlane2, m_srcRect);
	if (!m_bSWRendering)
	{
		m_bSWRendering = true;
		UpdateStatsStatic();
	}
	
	return S_OK;
}

HRESULT CDX12VideoProcessor::ProcessSample(IMediaSample* pSample)
{
	HRESULT hr = S_OK;
	REFERENCE_TIME rtStart, rtEnd;

	if (m_pFilter->m_filterState == State_Stopped)
		assert(0);
	
	CheckSwapChain(m_windowRect);
	
	

	const long size = pSample->GetActualDataLength();
	
	if (FAILED(pSample->GetTime(&rtStart, &rtEnd)))
		rtStart = m_pFilter->m_FrameStats.GeTimestamp();
	
	const REFERENCE_TIME rtFrameDur = m_pFilter->m_FrameStats.GetAverageFrameDuration();
	rtEnd = rtStart + rtFrameDur;

	m_rtStart = rtStart;
	CRefTime rtClock(rtStart);
	//copy sample via the videocopycontext
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
}

void CDX12VideoProcessor::UpdateDisplayInfo(const DisplayConfig_t& dc)
{
	std::wstring str;
	m_strStatsDispInfo = L"Display ";
	if (dc.width && dc.height && dc.refreshRate.Numerator) {
		double freq = (double)dc.refreshRate.Numerator / (double)dc.refreshRate.Denominator;
		str = fmt::format(L"{:.5f}", freq);
		if (dc.scanLineOrdering >= DISPLAYCONFIG_SCANLINE_ORDERING_INTERLACED)
			str += 'i';
		str.append(L" Hz");
	}
	if (str.size()) {
		m_strStatsDispInfo.append(str);
		str.clear();

		if (dc.bitsPerChannel) { // if bitsPerChannel is not set then colorEncoding and other values are invalid
			const wchar_t* colenc = ColorEncodingToString(dc.colorEncoding);
			if (colenc) {
				str = fmt::format(L" Color: {} {}-bit", colenc, dc.bitsPerChannel);
				if (dc.advancedColor.advancedColorSupported) {
					str.append(L" HDR10: ");
					str.append(dc.advancedColor.advancedColorEnabled ? L"on" : L"off");
				}
			}
		}
		m_strStatsDispInfo.append(str);
	}
	else
		m_strStatsDispInfo.append(D3DDisplayModeToString(*m_pDisplayMode));
}

HRESULT CDX12VideoProcessor::SetDevice(ID3D12Device* pDevice, const bool bDecoderDevice)
{
	D3D12Engine::g_Device = pDevice;
	//now the device is set get the caps of it
	D3D12Engine::FillD3D12Capabilities();
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
	if (m_decExFmt.value == 0)
		m_decExFmt = m_srcExFmt;
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
	if (!ProcessHDR(m_srcExFmt, m_srcParams))
	{
		D3D12Engine::ReleaseSwapChain();
		Init(m_hWnd);
	}
	m_srcWidth = origW;
	m_srcHeight = origH;
	
	m_iSrcFromGPU = 12;/* this will add D3D12 in the stats*/
	//we dont use the video processor yet but at least set the src height and width

#ifdef TODO
	HRESULT hr = m_D3D12VP.InitVideoProcessor(m_srcParams.VP11Format, origW, origH,false,m_srcParams.VP11Format);
	if (FAILED(hr))
		DLog(L"failed initiating the d3d12 video processor");
	else
		m_D3D12VP.InitInputTextures();
#endif
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
	//Update the colors in case we are using sw for rendering
	SetShaderConvertColorParams(m_srcExFmt, m_srcParams, m_DXVA2ProcAmpValues);
	UpdateStatsStatic();
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
	//TODO
	//add config for internal scaler

	m_bForceD3D12 = config.D3D12Settings.bForceD3D12;
	
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
	if (config.bUseDither != m_bUseDither) {
		m_bUseDither = config.bUseDither;
		changeNumTextures = D3D12Engine::GetInternalFormat() != DXGI_FORMAT_B8G8R8A8_UNORM;
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

void CDX12VideoProcessor::ReleaseVP()
{
	DLog(L"CDX11VideoProcessor::ReleaseVP()");

	m_pFilter->ResetStreamingTimes2();
	m_RenderStats.Reset();

	/*m_TexSrcVideo.Release();
	m_TexConvertOutput.Release();
	m_TexResize.Release();
	m_TexsPostScale.Release();

	m_PSConvColorData.Release();
	*/
	m_D3D12VP.ReleaseVideoProcessor();
	m_strCorrection = nullptr;

	m_srcParams = {};
	//m_srcDXGIFormat = DXGI_FORMAT_UNKNOWN;
	m_srcDXVA2Format = D3DFMT_UNKNOWN;
	m_pConvertFn = nullptr;
	m_srcWidth = 0;
	m_srcHeight = 0;
}
void CDX12VideoProcessor::ReleaseDevice()
{
	DLog(L"CDX12VideoProcessor::ReleaseDevice()");
	ReleaseVP();
	m_D3D12VP.ReleaseVideoDevice();
	g_Device = nullptr;
	
	
}

bool CDX12VideoProcessor::HandleHDRToggle()
{
	return false;
}

void CDX12VideoProcessor::UpdateRenderRect()
{
	m_renderRect.IntersectRect(m_videoRect, m_windowRect);

	const int w2 = m_videoRect.Width();
	const int h2 = m_videoRect.Height();
	const int k = m_bInterpolateAt50pct ? 2 : 1;
	int w1, h1;
	if (m_iRotation == 90 || m_iRotation == 270) {
		w1 = m_srcRectHeight;
		h1 = m_srcRectWidth;
	}
	else {
		w1 = m_srcRectWidth;
		h1 = m_srcRectHeight;
	}


	m_sScalerX = (w1 == w2) ? L""
		: (w1 > k * w2)
		? g_D3D12Options->GetCurrentDownscaler().c_str()
		: g_D3D12Options->GetCurrentUpscaler().c_str();
	m_sScalerY = (h1 == h2) ? L""
		: (h1 > k * h2)
			? g_D3D12Options->GetCurrentDownscaler().c_str()
			: g_D3D12Options->GetCurrentUpscaler().c_str();

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
		}
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





void CDX12VideoProcessor::DrawStats(GraphicsContext& Context, float x, float y, float w, float h)
{
	if (m_windowRect.IsRectEmpty())
		return;

	if (!m_bShowStats)
	{
		return;
	}

	GeometryContext BackgroundWindow(Context);
	BackgroundWindow.Begin();
	SIZE rtSize = m_windowRect.Size();
	//need to put the view port before rendering or we lose it outside the rendering window
	Context.SetViewportAndScissor(m_windowRect.left, m_windowRect.top, m_windowRect.Width(), m_windowRect.Height());
	BackgroundWindow.DrawRectangle(m_StatsRect, rtSize, D3DCOLOR_ARGB(80, 0, 0, 0));
	BackgroundWindow.End();

	std::wstring str;
	str.reserve(700);
	str.assign(m_strStatsHeader);
	str.append(m_strStatsDispInfo);

	//TODO interlace
	wchar_t frametype = (m_SampleFormat != D3D12_VIDEO_FIELD_TYPE_NONE) ? 'i' : 'p';
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
	str += fmt::format(L"\nScaling : {}x{} -> {}x{}", m_srcRectWidth, m_srcRectHeight, dstW, dstH);
	if (m_srcRectWidth != dstW || m_srcRectHeight != dstH)
	{
		str += L' ';
		const int w2 = m_videoRect.Width();
		const int h2 = m_videoRect.Height();
		const int k = m_bInterpolateAt50pct ? 2 : 1;
		int w1, h1;
		w1 = m_srcRectWidth;
		h1 = m_srcRectHeight;

		/*m_strShaderX = (w1 == w2) ? nullptr
			: (w1 > k * w2)
			? g_D3D12Options->GetCurrentDownscaler().c_str()
			: g_D3D12Options->GetCurrentUpscaler().c_str();
		m_strShaderY = (h1 == h2) ? nullptr
			: (h1 > k * h2)
			? g_D3D12Options->GetCurrentDownscaler().c_str()
			: g_D3D12Options->GetCurrentUpscaler().c_str();*/
		if (m_sScalerX.length()>0)
		{
			str.append(m_sScalerX);
			if (m_sScalerY.length() > 0 && m_sScalerY.length() != m_sScalerX.length())
			{
				str += L'/';
				str.append(m_sScalerY);
			}
		}
		else if (m_sScalerY.length() > 0)
			str.append(m_sScalerY);

	}
	str.append(m_strStatsHDR);
	str.append(m_strStatsPresent);
	str += fmt::format(L"\nFrames: {:5}, skipped: {}/{}, failed: {}",
		m_pFilter->m_FrameStats.GetFrames(), m_pFilter->m_DrawStats.m_dropped, m_RenderStats.dropped2, m_RenderStats.failed);

	if (m_pStatsDelay == 0)
	{
		m_pCurrentRenderTiming = fmt::format(L"\nTimings:");
		uint64_t ticks = GetPreciseTicksPerSecondI();
		float copy = (float)(m_RenderStats.copyticks * 1000) / ticks;
		m_pCurrentRenderTiming += fmt::format(L"\nCopy: {:.{}f}ms", copy, 2);
		float paint = (float)(m_RenderStats.paintticks * 1000) / ticks;
		m_pCurrentRenderTiming += fmt::format(L"\nPaint: {:.{}f}ms", paint, 2);
		if (m_bSubPicWasRendered)
		{
			float sub = (float)(m_RenderStats.substicks * 1000) / ticks;
			m_pCurrentRenderTiming += fmt::format(L"\nSubtitles: {:.{}f}ms", sub, 2);
		}
		float present = (float)(m_RenderStats.presentticks * 1000) / ticks;
		m_pCurrentRenderTiming += fmt::format(L"\nPresent: {:.{}f}ms BufferCount: {}", present, 2, g_CurrentBufferCount);
		m_pCurrentRenderTiming += fmt::format(L"\nSync offset   : {:+3} ms", (m_RenderStats.syncoffset + 5000) / 10000);
	}
	str.append(m_pCurrentRenderTiming);
	m_pStatsDelay += 1;
	if (m_pStatsDelay == 10)
		m_pStatsDelay = 0;
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
	//CheckPointer(m_pDXGISwapChain1, E_FAIL);
	HRESULT hr = S_OK;
	if (field)
		m_FieldDrawn = field;

	uint64_t tick1 = GetPreciseTick();

	HRESULT hrSubPic = E_FAIL;

	

	if (m_pFilter->m_pSubCallBack && m_pTextureSubPic.GetWidth()>0)
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

	GraphicsContext& pVideoContext = GraphicsContext::Begin(L"Render Video");

	//Clear our output resource
	if (!m_windowRect.IsRectEmpty())
		D3D12Engine::ClearBackBuffer(pVideoContext,m_windowRect);
	
	//copy the planes and scale if we have to
	if (!m_renderRect.IsRectEmpty())
		hr = Process(pVideoContext,m_srcRect, m_videoRect, m_FieldDrawn == 2);
	

	//Render the Subtitles
	if (S_OK == hrSubPic) {
		const CRect rSrcPri(CPoint(0, 0), m_windowRect.Size());

		D3D11_VIEWPORT VP;
		VP.TopLeftX = 0;
		VP.TopLeftY = 0;
		VP.Width = rSrcPri.Width();
		VP.Height = rSrcPri.Height();
		VP.MinDepth = 0.0f;
		VP.MaxDepth = 1.0f;
		hrSubPic = D3D12Engine::RenderSubPic(pVideoContext,m_pTextureSubPic, rSrcPri, m_d3dpp.BackBufferWidth, m_d3dpp.BackBufferHeight);
	}

	//will just desactive the overlay if stats are not on
	DrawStats(pVideoContext, 10, 10, m_windowRect.Width(), m_windowRect.Height());

	
	if (m_bAlphaBitmapEnable)
	{
		D3D12_RESOURCE_DESC desc = D3D12Engine::GetSwapChainResourceDesc();
		
		D3D12_VIEWPORT vp = {
			m_AlphaBitmapNRectDest.left * desc.Width,
			m_AlphaBitmapNRectDest.top * desc.Height,
			(m_AlphaBitmapNRectDest.right - m_AlphaBitmapNRectDest.left) * desc.Width,
			(m_AlphaBitmapNRectDest.bottom - m_AlphaBitmapNRectDest.top) * desc.Height,
			0.0f,
			1.0f
		};
		hrSubPic = D3D12Engine::RenderAlphaBitmap(pVideoContext, m_pAlphaBitmapTexture, vp, m_AlphaBitmapRectSrc);
#ifdef TODO
		hr = AlphaBlt(m_TexAlphaBitmap.pShaderResource, pBackBuffer, m_pAlphaBitmapVertex, &VP, m_pSamplerLinear);
#endif
	}
	//TODO ADD HDR METADATA

	D3D12Engine::PresentBackBuffer(pVideoContext);
	m_RenderStats.substicks = tick2 - tick1; // after DrawStats to relate to paintticks
	uint64_t tick3 = GetPreciseTick();
	m_RenderStats.paintticks = tick3 - tick1;
	pVideoContext.TransitionResource(D3D12Engine::GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);
	pVideoContext.Finish();
	
	
	

	
	D3D12Engine::WaitForVBlank();

	g_bPresent12 = true;
	D3D12Engine::Present();
	
	g_bPresent12 = false;
	DLogIf(FAILED(hr), L"CDX11VideoProcessor::Render() : Present() failed with error {}", HR2Str(hr));

	m_RenderStats.presentticks = GetPreciseTick() - tick3;
#ifdef TODO
	if (hr == DXGI_ERROR_INVALID_CALL && m_pFilter->m_bIsD3DFullscreen)
		InitSwapChain();
#endif
	return hr;
}

HRESULT CDX12VideoProcessor::Process(GraphicsContext& pVideoContext,const CRect& srcRect, const CRect& dstRect, const bool second)
{
	HRESULT hr = S_OK;
	m_bDitherUsed = false;
	int rotation = m_iRotation;
	CRect rSrc = srcRect;
	
	if (rSrc.Width() != dstRect.Width() && rSrc.Height() != dstRect.Height())
	{
		if (rSrc.Width() > dstRect.Width() || rSrc.Height() > dstRect.Height())
		{
			std::wstring scurrentscaler = D3D12Engine::g_D3D12Options->GetCurrentDownscaler();
			if (scurrentscaler != m_sScalerX)
				m_sScalerX = scurrentscaler;
			if (scurrentscaler != m_sScalerY)
				m_sScalerY = scurrentscaler;
			D3D12Engine::Downscale(pVideoContext, srcRect, dstRect, m_bSWRendering, scurrentscaler);
		}
		else
		{
			std::wstring scurrentscaler = D3D12Engine::g_D3D12Options->GetCurrentUpscaler();
			if (scurrentscaler != m_sScalerX)
				m_sScalerX = scurrentscaler;
			if (scurrentscaler != m_sScalerY)
				m_sScalerY = scurrentscaler;
			D3D12Engine::Upscale(pVideoContext, srcRect, dstRect, m_bSWRendering, scurrentscaler);
		}
	}
	else
	{
		//even with no scale we need a dstrect if we resize the window only in x or y position
		D3D12Engine::Noscale(pVideoContext, dstRect, m_bSWRendering);
	}
	return hr;
}

HRESULT CDX12VideoProcessor::FillBlack()
{
    return S_OK;
}

void CDX12VideoProcessor::SetVideoRect(const CRect& videoRect)
{
	m_videoRect = videoRect;
	UpdateRenderRect();
}

HRESULT CDX12VideoProcessor::SetWindowRect(const CRect& windowRect)
{
	
	m_windowRect = windowRect;
	m_renderRect.IntersectRect(m_videoRect, m_windowRect);

	D3D12Engine::ResetSwapChain(m_windowRect, g_D3D12Options->GetCurrentPresentBufferCount());
	D3D12Engine::SetVideoRect(m_videoRect);
	D3D12Engine::SetWindowRect(m_windowRect);
	D3D12Engine::SetRenderRect(m_renderRect);
	m_VendorId = g_VendorId;
	
	SetGraphSize();
	UpdateStatsStatic();
	return S_OK;
}

HRESULT CDX12VideoProcessor::Reset()
{
	//not sure about this
	return D3D12Engine::ResetSwapChain(m_windowRect, g_D3D12Options->GetCurrentPresentBufferCount());
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
	if (m_srcParams.cformat)
	{
		m_strStatsHeader = fmt::format(L"MPC VR, D3D12 ");
		if (m_bSWRendering)
			m_strStatsHeader += L"Sw to d3d12 Texture\n";
		else
			m_strStatsHeader += L"hw D3D12 texture input\n";
		UpdateStatsInputFmt();

		m_strStatsVProc = fmt::format(L"\nInternalFormat: {}", DXGIFormatToString(D3D12Engine::GetInternalFormat()));

		if (SourceIsHDR()) {
			m_strStatsHDR.assign(L"\nHDR processing: ");
			if (D3D12Engine::HdrPassthroughSupport() && m_bHdrPassthrough) {
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
	DLog(L"CDX12VideoProcessor::SetProcAmpValues");
  return E_NOTIMPL;
}

STDMETHODIMP_(HRESULT __stdcall) CDX12VideoProcessor::SetAlphaBitmap(const MFVideoAlphaBitmap* pBmpParms)
{
	DLog(L"CDX12VideoProcessor::SetAlphaBitmap");
	CheckPointer(pBmpParms, E_POINTER);

	CAutoLock cRendererLock(&m_pFilter->m_RendererLock);
	HRESULT hr = S_FALSE;

	if (pBmpParms->GetBitmapFromDC && pBmpParms->bitmap.hdc) {
		HBITMAP hBitmap = (HBITMAP)GetCurrentObject(pBmpParms->bitmap.hdc, OBJ_BITMAP);
		if (!hBitmap) {
			return E_INVALIDARG;
		}
		DIBSECTION info = { 0 };
		if (!::GetObjectW(hBitmap, sizeof(DIBSECTION), &info)) {
			return E_INVALIDARG;
		}
		BITMAP& bm = info.dsBm;
		if (!bm.bmWidth || !bm.bmHeight || bm.bmBitsPixel != 32 || !bm.bmBits) {
			return E_INVALIDARG;
		}
		if (m_pAlphaBitmapTexture.GetWidth() == 0 || m_pAlphaBitmapTexture.GetWidth() != bm.bmWidth, m_pAlphaBitmapTexture.GetHeight() != bm.bmHeight)
		{
			if (m_pAlphaBitmapTexture.GetWidth() != 0)
				m_pAlphaBitmapTexture.Destroy();
			GraphicsContext& pVideoContext = GraphicsContext::Begin(L"AlphabitmapCreate");
			m_pAlphaBitmapTexture.Create2D(bm.bmWidthBytes, bm.bmWidth, bm.bmHeight, DXGI_FORMAT_B8G8R8A8_UNORM, bm.bmBits);
			pVideoContext.Finish(true);
			
			
		}
		else
		{
			D3D12_SUBRESOURCE_DATA texResource;
			texResource.pData = bm.bmBits;
			texResource.RowPitch = bm.bmWidthBytes;
			texResource.SlicePitch = bm.bmWidthBytes * bm.bmHeight;
			//need to transit the resource if we don't have a state error
			GraphicsContext& pVideoContext = GraphicsContext::Begin(L"Alphabitmaptransit");
			pVideoContext.TransitionResource(m_pAlphaBitmapTexture, D3D12_RESOURCE_STATE_COPY_DEST);
			pVideoContext.Finish(true);
			
			CommandContext::InitializeTexture(m_pAlphaBitmapTexture, 1, &texResource);
		}
		hr = S_OK;
	}
	else
		return E_INVALIDARG;
	
	m_bAlphaBitmapEnable = (m_pAlphaBitmapTexture.GetWidth() > 0);

	if (m_bAlphaBitmapEnable) {
		m_AlphaBitmapRectSrc= { 0, 0, (LONG)m_pAlphaBitmapTexture.GetWidth(), (LONG)m_pAlphaBitmapTexture.GetHeight()};
		m_AlphaBitmapNRectDest = { 0, 0, 1, 1 };
		hr = UpdateAlphaBitmapParameters(&pBmpParms->params);
	}

  return hr;
}

STDMETHODIMP_(HRESULT __stdcall) CDX12VideoProcessor::UpdateAlphaBitmapParameters(const MFVideoAlphaBitmapParams* pBmpParms)
{
	

	CheckPointer(pBmpParms, E_POINTER);
	CAutoLock cRendererLock(&m_pFilter->m_RendererLock);

	if (m_bAlphaBitmapEnable)
	{
		if (pBmpParms->dwFlags & MFVideoAlphaBitmap_SrcRect)
			m_AlphaBitmapRectSrc = pBmpParms->rcSrc;

		if (pBmpParms->dwFlags & MFVideoAlphaBitmap_DestRect)
			m_AlphaBitmapNRectDest = pBmpParms->nrcDest;
		
		DWORD validFlags = MFVideoAlphaBitmap_SrcRect | MFVideoAlphaBitmap_DestRect;

		return ((pBmpParms->dwFlags & validFlags) == validFlags) ? S_OK : S_FALSE;
	}
	else 
		return MF_E_NOT_INITIALIZED;
}


void CDX12VideoProcessor::SetCallbackDevice(const bool bChangeDevice/* = false*/)
{
	if ((!m_bCallbackDeviceIsSet || bChangeDevice) && m_pD3DDevEx && m_pFilter->m_pSubCallBack) {
		m_bCallbackDeviceIsSet = SUCCEEDED(m_pFilter->m_pSubCallBack->SetDevice(m_pD3DDevEx));
	}
}