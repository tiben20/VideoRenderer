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

#include <dxgi1_6.h>
#include "Helper.h"
#include "DX12Engine.h"
#include <string>
#include "CommandListManager.h"
#include "BufferManager.h"

#include "DisplayConfig.h"
#include <map>

namespace D3D12Engine
{
	//using namespace ImageScaling;

	ID3D12Device* g_Device = nullptr;
	CommandListManager g_CommandManager;
	ContextManager g_ContextManager;

	HWND g_hWnd = nullptr;
	DWORD g_VendorId = 0;

	CRect g_videoRect = CRect();
	CRect g_windowRect = CRect();
	CRect g_renderRect = CRect();
	DescriptorAllocator g_DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] =
	{
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV
	};


	//Dxgi device and swapchain
	CComPtr<IDXGIAdapter> m_pDXGIAdapter;
	CComPtr<IDXGIFactory2>   m_pDXGIFactory2;
	CComPtr<IDXGISwapChain1> m_pDXGISwapChain1;
	CComPtr<IDXGISwapChain4> m_pDXGISwapChain4;
	CComPtr<IDXGIOutput>    m_pDXGIOutput;
	CComPtr<IDXGIFactory1> m_pDXGIFactory1;

	ID3D12Resource* SwapChainBuffer[3];
	ColorBuffer SwapChainBufferColor[3];
	ColorBuffer m_pResizeResource;// same format as back buffer,will have the plane rendered onto
	ColorBuffer m_pVideoOutputResource;// same format as back buffer,will have the plane rendered onto
	ColorBuffer m_pPlaneResource[2];//Those surface are for copy texture from nv12 to rgb
	int p_CurrentBuffer = 0;
	DXGI_COLOR_SPACE_TYPE m_currentSwapChainColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
	// swap chain format
	//DXGI_FORMAT m_SwapChainFmt = DXGI_FORMAT_R10G10B10A2_UNORM;
	DXGI_FORMAT m_SwapChainFmt = DXGI_FORMAT_R8G8B8A8_UNORM;
	bool m_bIsFullscreen = false;
	// hdr
	std::map<std::wstring, BOOL> m_hdrModeSavedState;
	std::map<std::wstring, BOOL> m_hdrModeStartState;
	bool m_bHdrPassthroughSupport = false;
	bool m_bHdrDisplaySwitching = false; // switching HDR display in progress
	bool m_bHdrDisplayModeEnabled = false;
	bool m_bHdrAllowSwitchDisplay = true;
	UINT m_srcVideoTransferFunction = 0; // need a description or rename
	UINT32 m_bitsPerChannelSupport = 8;

	std::wstring m_strAdapterDescription;

	// Input parameters
	DXGI_FORMAT m_srcDXGIFormat = DXGI_FORMAT_UNKNOWN;

	// intermediate texture format
	DXGI_FORMAT m_InternalTexFmt = DXGI_FORMAT_B8G8R8A8_UNORM;

	CONSTANT_BUFFER_VAR m_pBufferVar;
	bool m_PSConvColorData = false;

	std::wstring D3D12Engine::GetAdapterDescription() { return m_strAdapterDescription; }
	DXGI_FORMAT D3D12Engine::GetSwapChainFormat() { return m_SwapChainFmt; }
	DXGI_FORMAT D3D12Engine::GetInternalFormat() { return m_InternalTexFmt; }
	D3D12_RESOURCE_DESC D3D12Engine::GetSwapChainResourceDesc() { return SwapChainBufferColor[0]->GetDesc(); }
	bool D3D12Engine::HdrPassthroughSupport() { return m_bHdrPassthroughSupport; }
	IDXGIFactory1* D3D12Engine::GetDXGIFactory() { return m_pDXGIFactory1; }
	
	void SetVideoRect(CRect rect) { g_videoRect = rect; }
	void SetWindowRect(CRect rect) { g_windowRect = rect; }
	void SetRenderRect(CRect rect) { g_renderRect = rect; }

	bool Preferred10BitOutput()
	{
		return m_bitsPerChannelSupport >= 10 && (GetInternalFormat() == DXGI_FORMAT_R10G10B10A2_UNORM || GetInternalFormat() == DXGI_FORMAT_R16G16B16A16_FLOAT);
	}

	void D3D12Engine::ReleaseSwapChain()
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

	void D3D12Engine::ReleaseEngine()
	{
		/*Need this to have correct heap if we dont entirely close the module*/
		g_DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		g_DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		g_DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		g_DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] = DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);


		m_pPlaneResource[0].DestroyBuffer();
		m_pPlaneResource[1].DestroyBuffer();
		m_pResizeResource.DestroyBuffer();
		m_pVideoOutputResource.DestroyBuffer();

		g_hWnd = nullptr;
		m_pDXGIAdapter.Release();
		m_pDXGIFactory2.Release();
		m_pDXGIFactory1.Release();
		m_pDXGIOutput.Release();
		p_CurrentBuffer = 0;

	}

	void D3D12Engine::CheckSwapChain(CRect windowrect)
	{
		DXGI_SWAP_CHAIN_DESC desc;
		if (!m_pDXGISwapChain1)
			InitSwapChain(windowrect);
		m_pDXGISwapChain1->GetDesc(&desc);
		if (desc.OutputWindow != g_hWnd)
			assert(0);
			//Reset();
	}

	HRESULT D3D12Engine::ResetSwapChain(CRect windowrect)
	{
		const UINT w = windowrect.Width();
		const UINT h = windowrect.Height();
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
				CComPtr<ID3D12Resource> DisplayPlane;

				for (uint32_t i = 0; i < 3; ++i)
				{
					CComPtr<ID3D12Resource> DisplayPlane;
					m_pDXGISwapChain4->GetBuffer(i, MY_IID_PPV_ARGS(&DisplayPlane));
					SwapChainBufferColor[i].CreateFromSwapChain(L"Primary SwapChain Buffer", DisplayPlane.Detach());
				}
				D3D12Engine::g_OverlayBuffer.Create(L"UI Overlay", w, h, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
				D3D12Engine::g_HorizontalBuffer.Create(L"Bicubic Intermediate", w, h, 1, DXGI_FORMAT_R10G10B10A2_UNORM);
				p_CurrentBuffer = 0;
				D3D12Engine::g_CommandManager.IdleGPU();
			}
		}
		return S_OK;
	}

	HRESULT D3D12Engine::CreateDevice()
	{
		HRESULT hr = S_OK;
		ID3D12Debug* pD3DDebug;
		ID3D12Debug1* pD3DDebug1;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pD3DDebug))))
		{
			pD3DDebug->EnableDebugLayer();
			pD3DDebug->QueryInterface(IID_PPV_ARGS(&pD3DDebug1));
			//m_pD3DDebug1->SetEnableGPUBasedValidation(true);
			pD3DDebug1->SetEnableSynchronizedCommandQueueValidation(1);
			pD3DDebug->Release();
			pD3DDebug1->Release();
		}
		
#if 0
		// create DXGI factory
		IDXGIAdapter* pDXGIAdapter = nullptr;
		IDXGIFactory1* pDXGIFactory = nullptr;
		hr = dx.mCreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_pDxgiFactory));
		if (FAILED(hr))
		{
			DbgLog((LOG_ERROR, 10, L"-> DXGIFactory creation failed"));
			//goto fail;
		}

		//hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_pDxgiFactory));
		if FAILED(hr)
			assert(0);

		if (DXGI_ERROR_NOT_FOUND == m_pDxgiFactory->EnumAdapters(nDeviceIndex, &m_pDxgiAdapter))
		{
			hr = S_FALSE;
			assert(0);
		}
		DXGI_ADAPTER_DESC desc;
		hr = m_pDxgiAdapter->GetDesc(&desc);
		if FAILED(hr)
			assert(0);
#endif
		const D3D_FEATURE_LEVEL* levels = NULL;
		D3D_FEATURE_LEVEL max_level = D3D_FEATURE_LEVEL_12_1;
		//int level_count = s_GetD3D12FeatureLevels(max_level, &levels);

		hr = D3D12CreateDevice(m_pDXGIAdapter, max_level, IID_PPV_ARGS(&g_Device));
		if FAILED(hr)
			assert(0);



		//hr = m_pD3DDevice->QueryInterface(IID_PPV_ARGS(&m_pVideoDevice));
		return hr;

	}

	HRESULT D3D12Engine::InitSwapChain(CRect windowRect)
	{
		for (int i = 0; i < 3; i++)
		{
			SwapChainBufferColor[i].Destroy();
			if (SwapChainBuffer[i])
				SwapChainBuffer[i]->Release();
		}

		m_pDXGISwapChain1.Release();

		CComPtr<IDXGIFactory4> dxgiFactory;
		EXECUTE_ASSERT(S_OK == CreateDXGIFactory2(0, MY_IID_PPV_ARGS(&dxgiFactory)));

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = windowRect.Width();
		swapChainDesc.Height = windowRect.Height();
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

		if (desc2.OutputWindow != g_hWnd)
			assert(0);
		
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
							g_VendorId = dxgiAdapterDesc.VendorId;
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

		return S_OK;
	}

	HRESULT D3D12Engine::InitDXGIFactory()
	{
		HRESULT hr = CreateDXGIFactory1(IID_IDXGIFactory1, (void**)&m_pDXGIFactory1);
		if (FAILED(hr)) {
			DLog(L"CDX12VideoProcessor::CDX12VideoProcessor() : CreateDXGIFactory1() failed with error {}", HR2Str(hr));

		}
		return hr;
	}

	bool D3D12Engine::ProcessHDR( DXVA2_ExtendedFormat srcExFmt, FmtConvParams_t m_srcParams)
	{
		bool reinit = false;
		if (m_bHdrAllowSwitchDisplay && m_srcVideoTransferFunction != srcExFmt.VideoTransferFunction) {
			//the handlehdrtoggle is not writen yet
#if TODO
			auto ret = HandleHDRToggle();
			if (!ret && (m_bHdrPassthrough && m_bHdrPassthroughSupport && SourceIsHDR() && !m_pDXGISwapChain4)) 
				ret = true;
			if (ret) {
				//ReleaseSwapChain();
				reinit = true;
				//Init(hwnd);
			}
#endif
		}

		if (Preferred10BitOutput() && m_SwapChainFmt == DXGI_FORMAT_B8G8R8A8_UNORM) {
			reinit = true;
			//ReleaseSwapChain();
			//Init(hwnd);
		}


		m_srcVideoTransferFunction = srcExFmt.VideoTransferFunction;
		
		m_srcDXGIFormat = m_srcParams.VP11Format;
		return S_OK;
	}

	void D3D12Engine::SetShaderConvertColorParams(DXVA2_ExtendedFormat srcExFmt, FmtConvParams_t m_srcParams, DXVA2_ProcAmpValues m_DXVA2ProcAmpValues)
	{
		mp_csp_params csp_params;
		set_colorspace(srcExFmt, csp_params.color);
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

	HRESULT D3D12Engine::RenderSubPic(GraphicsContext& Context, ColorBuffer resource, CRect srcRect)
	{
		return ImageScaling::RenderSubPic(Context, resource, g_OverlayBuffer, srcRect);
		//return ImageScaling::RenderSubPic(Context, resource, SwapChainBufferColor[p_CurrentBuffer], srcRect);
	}

	HRESULT D3D12Engine::CopySampleSW(TypedBuffer buf1, TypedBuffer buf2, D3D12_PLACED_SUBRESOURCE_FOOTPRINT layoutplane[2])
	{
		//create the resize resource needed to wait to have a video source
		if (m_pResizeResource.GetWidth() == 0)
		{
			m_pResizeResource.Create(L"Resize Scaling Resource", layoutplane[0].Footprint.Width, layoutplane[0].Footprint.Height, 1, m_SwapChainFmt);
			m_pVideoOutputResource.Create(L"Resize Scaling Resource Final", layoutplane[0].Footprint.Width, layoutplane[0].Footprint.Height, 1, m_SwapChainFmt);
		}

		if (m_pPlaneResource[0].GetWidth() == 0)// DXGI_FORMAT_NV12
		{
			m_pPlaneResource[0].Create(L"Scaling Resource", layoutplane[0].Footprint.Width, layoutplane[0].Footprint.Height, 1, layoutplane[0].Footprint.Format);
			m_pPlaneResource[1].Create(L"Scaling Resource", layoutplane[1].Footprint.Width, layoutplane[1].Footprint.Height, 1, layoutplane[1].Footprint.Format);
			if (DXGI_FORMAT_R16_UNORM == layoutplane[0].Footprint.Format)
				m_srcDXGIFormat = DXGI_FORMAT_P010;
			else
				m_srcDXGIFormat = DXGI_FORMAT_NV12;
		}
		/*else if (m_pPlaneResource[0].GetWidth() == 0)
		{
			m_pPlaneResource[0].Create(L"Scaling Resource", layoutplane[0].Footprint.Width, layoutplane[0].Footprint.Height, 1, DXGI_FORMAT_R16_UNORM);
			m_pPlaneResource[1].Create(L"Scaling Resource", layoutplane[1].Footprint.Width, layoutplane[1].Footprint.Height, 1, DXGI_FORMAT_R16G16_UNORM);
			m_srcDXGIFormat = DXGI_FORMAT_P010;
		}*/
		GraphicsContext& pVideoContext = GraphicsContext::Begin(L"Render Video");
		pVideoContext.TransitionResource(m_pPlaneResource[0], D3D12_RESOURCE_STATE_COPY_DEST);
		pVideoContext.TransitionResource(m_pPlaneResource[1], D3D12_RESOURCE_STATE_COPY_DEST, true);
		
		D3D12_TEXTURE_COPY_LOCATION dst;
		D3D12_TEXTURE_COPY_LOCATION src;
		for (int i = 0; i < 2; i++) {
			dst = CD3DX12_TEXTURE_COPY_LOCATION(m_pPlaneResource[i].GetResource());
			if (i==0)
			src = CD3DX12_TEXTURE_COPY_LOCATION(buf1.GetResource(), layoutplane[i]);
			else
				src = CD3DX12_TEXTURE_COPY_LOCATION(buf2.GetResource(), layoutplane[i]);
			pVideoContext.GetCommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
		}

		//the first plane is the correct size not the second one
		pVideoContext.SetViewportAndScissor(0, 0, layoutplane[0].Footprint.Width, layoutplane[0].Footprint.Height);
		ImageScaling::ColorAjust(pVideoContext, m_pResizeResource, m_pPlaneResource[0], m_pPlaneResource[1], m_pBufferVar);
		pVideoContext.Finish(true);
		return S_OK;
	
	}

	HRESULT D3D12Engine::CopySample(ID3D12Resource* resource)
	{
		D3D12_RESOURCE_DESC desc;
		desc = resource->GetDesc();



		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layoutplane[2];
		UINT64 pitch_plane[2];
		UINT rows_plane[2];
		UINT64 RequiredSize;
		//m_renderRect.IntersectRect(m_videoRect, m_windowRect);
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
		for (int i = 0; i < 2; i++)
		{
			dst = CD3DX12_TEXTURE_COPY_LOCATION(m_pPlaneResource[i].GetResource());
			src = CD3DX12_TEXTURE_COPY_LOCATION(resource, i);
			pVideoContext.GetCommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
		}
		//the first plane is the correct size not the second one
		pVideoContext.SetViewportAndScissor(0, 0, layoutplane[0].Footprint.Width, layoutplane[0].Footprint.Height);
		
		ImageScaling::ColorAjust(pVideoContext, m_pResizeResource, m_pPlaneResource[0], m_pPlaneResource[1], m_pBufferVar);
		pVideoContext.Finish();
		return S_OK;
	}

	void D3D12Engine::ClearBackBuffer(CRect windowRect)
	{
		GraphicsContext& pVideoContext = GraphicsContext::Begin(L"Clear Target");
		pVideoContext.SetViewportAndScissor(windowRect.left, windowRect.top, windowRect.Width(), windowRect.Height());
		pVideoContext.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
		pVideoContext.SetRenderTarget(SwapChainBufferColor[p_CurrentBuffer].GetRTV());

		pVideoContext.ClearColor(SwapChainBufferColor[p_CurrentBuffer]);
		pVideoContext.Finish();
	}

	void D3D12Engine::Downscale(GraphicsContext& Context, int ScalingFilter, CRect srcRect, CRect destRect)
	{
		
		ImageScaling::Downscale(Context, m_pVideoOutputResource, m_pResizeResource, ScalingFilter, srcRect, destRect);
		Context.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);

		ImageScaling::PreparePresentSDR(Context, SwapChainBufferColor[p_CurrentBuffer], m_pVideoOutputResource, srcRect);
		Context.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_PRESENT);
	}

	void D3D12Engine::Upscale(GraphicsContext& Context, int ScalingFilter, CRect srcRect, CRect destRect)
	{
		
		ImageScaling::Upscale(Context, m_pVideoOutputResource, m_pResizeResource, ScalingFilter, srcRect, destRect);
		Context.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
		//by using g_videoRect as CRect renderrect we get the clear is ok but we lose the overlay which is rendered in the full window
		ImageScaling::PreparePresentSDR(Context, SwapChainBufferColor[p_CurrentBuffer], m_pVideoOutputResource, g_videoRect);
		Context.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_PRESENT);
	}

	void D3D12Engine::PresentBackBuffer(GraphicsContext& Context)
	{
		Context.SetViewportAndScissor(g_windowRect.left, g_windowRect.top, g_windowRect.Width(), g_windowRect.Height());
		Context.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
		ImageScaling::PreparePresentSDR(Context, SwapChainBufferColor[p_CurrentBuffer], m_pResizeResource, g_videoRect);
		Context.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_PRESENT);
	}

	void D3D12Engine::WaitForVBlank()
	{
		if (m_pDXGIOutput) {
			HRESULT hr = m_pDXGIOutput->WaitForVBlank();
			DLogIf(FAILED(hr), L"WaitForVBlank failed with error {}", HR2Str(hr));
		}
	}

	void D3D12Engine::Present()
	{
		
		m_pDXGISwapChain1->Present(1, 0);
		p_CurrentBuffer = (p_CurrentBuffer + 1) % 3;
	
	}

}/*D3D12Engine End*/