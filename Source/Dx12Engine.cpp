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
#include "DisplayConfig.h"
#include <map>
#ifndef UNLOAD_SCALER
#define UNLOAD_SCALER(x) if (x) {x->Unload();} x = nullptr;
#endif

namespace D3D12Engine
{
	//using namespace ImageScaling;

	ID3D12Device* g_Device = nullptr;
	CD3D12Options* g_D3D12Options;
	CommandListManager g_CommandManager;
	ContextManager g_ContextManager;
	CD3D12DynamicScaler* m_pCurrentUpScaler = nullptr;
	CD3D12DynamicScaler* m_pCurrentDownScaler = nullptr;
	CD3D12DynamicScaler* m_pCurrentChromaScaler = nullptr;
	CD3D12DynamicScaler* m_pCurrentImageDoubler = nullptr;
	RootSignature g_RootScalers;
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

	
	std::vector<ColorBuffer> SwapChainBufferColor;
	//ColorBuffer m_pVideoOutputResourcePreScale;// same format as back buffer,will have the plane rendered onto
	//ColorBuffer m_pVideoOutputResource;// same format as back buffer,will have the plane rendered onto
	//ColorBuffer m_pPlaneResource[2];//Those surface are for copy texture from nv12 to rgb
	int p_CurrentBuffer = 0;
	int g_CurrentBufferCount = 3;
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

	CONSTANT_BUFFER_VAR m_pBufferVar2;
	bool m_PSConvColorData2 = false;

	//ColorBuffer D3D12Engine::GetPreScale() {return m_pVideoOutputResourcePreScale;}
	
	ColorBuffer D3D12Engine::GetCurrentBackBuffer() { return SwapChainBufferColor[p_CurrentBuffer]; }
	std::wstring D3D12Engine::GetAdapterDescription() { return m_strAdapterDescription; }
	DXGI_FORMAT D3D12Engine::GetSwapChainFormat() { return m_SwapChainFmt; }
	DXGI_FORMAT D3D12Engine::GetInternalFormat() { return m_InternalTexFmt; }
	D3D12_RESOURCE_DESC D3D12Engine::GetSwapChainResourceDesc() { return SwapChainBufferColor[0]->GetDesc(); }
	bool D3D12Engine::HdrPassthroughSupport() { return m_bHdrPassthroughSupport; }
	IDXGIFactory1* D3D12Engine::GetDXGIFactory() { return m_pDXGIFactory1; }
	
	void SetVideoRect(CRect rect) { g_videoRect = rect; }
	void SetRenderRect(CRect rect) { g_renderRect = rect; }
	void SetWindowRect(CRect rect)
	{
		//need to update to the window size
		//DXGI_FORMAT fmt = m_pVideoOutputResource.GetFormat();
		/*m_pVideoOutputResource.DestroyBuffer();
		if (m_pVideoOutputResourcePreScale.GetWidth() == 0)
		{
			//once set don't touch it its the size of the video
			m_pVideoOutputResourcePreScale.DestroyBuffer();
			m_pVideoOutputResourcePreScale.Create(L"PreOutput video resource", g_videoRect.Width(), g_videoRect.Height(), 1, m_SwapChainFmt);
		}
		
		m_pVideoOutputResource.Create(L"Output video resource", rect.Width(), rect.Height(), 1, m_SwapChainFmt);*/
		g_windowRect = rect;

	}

	bool Preferred10BitOutput()
	{
		return m_bitsPerChannelSupport >= 10 && (GetInternalFormat() == DXGI_FORMAT_R10G10B10A2_UNORM || GetInternalFormat() == DXGI_FORMAT_R16G16B16A16_FLOAT);
	}

	void D3D12Engine::ReleaseSwapChain()
	{
		for (uint32_t i = 0; i < SwapChainBufferColor.size(); ++i)
		{
			SwapChainBufferColor.at(i).Destroy();
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

		UNLOAD_SCALER(m_pCurrentUpScaler)
		UNLOAD_SCALER(m_pCurrentDownScaler)
		UNLOAD_SCALER(m_pCurrentChromaScaler)
		UNLOAD_SCALER(m_pCurrentImageDoubler)
		/*Need this to have correct heap if we dont entirely close the module*/
		g_DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		g_DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		g_DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		g_DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] = DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);


		//m_pPlaneResource[0].DestroyBuffer();
		//m_pPlaneResource[1].DestroyBuffer();
		//m_pVideoOutputResourcePreScale.DestroyBuffer();
		//m_pVideoOutputResource.DestroyBuffer();
		
		g_hWnd = nullptr;
		m_pDXGIAdapter.Release();
		m_pDXGIFactory2.Release();
		m_pDXGIFactory1.Release();
		m_pDXGIOutput.Release();
		p_CurrentBuffer = 0;
		g_D3D12Options = nullptr;

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

	HRESULT D3D12Engine::ResetSwapChain(CRect windowrect, int presentSurfaceCount)
	{
		g_CurrentBufferCount = presentSurfaceCount;
		const UINT w = windowrect.Width();
		const UINT h = windowrect.Height();
		if (m_pDXGISwapChain1)
		{
			D3D12Engine::g_CommandManager.IdleGPU();
			for (uint32_t i = 0; i < SwapChainBufferColor.size(); ++i)
			{
				SwapChainBufferColor[i].Destroy();
			}
			SwapChainBufferColor.resize(presentSurfaceCount);
			if (!m_pDXGISwapChain4)
				assert(0);
			HRESULT hr = m_pDXGISwapChain4->ResizeBuffers(presentSurfaceCount, w, h, m_SwapChainFmt, 0);
			if (SUCCEEDED(hr))
			{
				CComPtr<ID3D12Resource> DisplayPlane;

				for (uint32_t i = 0; i < presentSurfaceCount; ++i)
				{
					CComPtr<ID3D12Resource> DisplayPlane;
					m_pDXGISwapChain4->GetBuffer(i, MY_IID_PPV_ARGS(&DisplayPlane));
					SwapChainBufferColor[i].CreateFromSwapChain(L"Primary SwapChain Buffer", DisplayPlane.Detach());
				}
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
			pD3DDebug1->SetEnableSynchronizedCommandQueueValidation(1);
			pD3DDebug->Release();
			pD3DDebug1->Release();
		}
		
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
		m_pDXGISwapChain1.Release();

		CComPtr<IDXGIFactory4> dxgiFactory;
		EXECUTE_ASSERT(S_OK == CreateDXGIFactory2(0, MY_IID_PPV_ARGS(&dxgiFactory)));

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = windowRect.Width();
		swapChainDesc.Height = windowRect.Height();
		swapChainDesc.Format = m_SwapChainFmt;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = g_D3D12Options->GetCurrentPresentBufferCount();
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Scaling = DXGI_SCALING_NONE;
		//d3d12 only support DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL or DXGI_SWAP_EFFECT_FLIP_DISCARD
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
		//Create ColorBuffer swap chain
		if (FAILED(ResetSwapChain(windowRect, swapChainDesc.BufferCount)))
			assert(0);

		hr = m_pDXGISwapChain1->GetContainingOutput(&m_pDXGIOutput);
		dxgiFactory.Release();
		DXGI_SWAP_CHAIN_DESC desc2;
		m_pDXGISwapChain1->GetDesc(&desc2);

		if (desc2.OutputWindow != g_hWnd)
			assert(0);
		
		//extract the data for the stats overlay
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

		return S_OK;
	}

	HRESULT D3D12Engine::InitDXGIFactory()
	{
		HRESULT hr = CreateDXGIFactory1(IID_IDXGIFactory1, (void**)&m_pDXGIFactory1);
		if (FAILED(hr)) {
			DLog(L"CDX12VideoProcessor::CDX12VideoProcessor() : CreateDXGIFactory1() failed with error {}", HR2Str(hr));

		}
		if (!g_D3D12Options)
			g_D3D12Options = new CD3D12Options();
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

	

	HRESULT D3D12Engine::RenderSubPic(GraphicsContext& Context, ColorBuffer resource, CRect srcRect, UINT srcW, UINT srcH)
	{
		//ImageScaling::RenderSubPic(Context, resource, m_pVideoOutputResource, srcRect, srcW,srcH);
		Context.SetViewportAndScissor(g_windowRect.left, g_windowRect.top, g_windowRect.Width(), g_windowRect.Height());
		return S_OK;
	}

	HRESULT D3D12Engine::RenderAlphaBitmap(GraphicsContext& Context, Texture resource, D3D12_VIEWPORT vp, RECT alphaBitmapRectSrc)
	{
		Context.SetViewportAndScissor(vp.TopLeftX, vp.TopLeftY, vp.Width, vp.Height);
		ImageScaling::RenderAlphaBitmap(Context, resource, alphaBitmapRectSrc);
		//need to go back to where we were
		Context.SetViewportAndScissor(g_windowRect.left, g_windowRect.top, g_windowRect.Width(), g_windowRect.Height());
		
		return S_OK;
	}
	
	HRESULT D3D12Engine::CopySampleShaderPassSW(Texture buf1, Texture buf2,CRect dstRect)
	{
		/*
		GraphicsContext& pShaderContext = GraphicsContext::Begin(L"Shader Context");
		if (m_pVideoOutputResourcePreScale.GetWidth() != dstRect.Width() || m_pVideoOutputResourcePreScale.GetHeight() != dstRect.Height())
		{
			//somehow we need to remake the output resource
			//only happen on pal video
			m_pVideoOutputResourcePreScale.DestroyBuffer();
			m_pVideoOutputResourcePreScale.Create(L"Resize Scaling Resource", dstRect.Width(), dstRect.Height(), 1, m_SwapChainFmt);
		}
		ImageScaling::CopyPlanesSW(pShaderContext, m_pVideoOutputResourcePreScale, buf1, buf2, m_pBufferVar2, dstRect);

		//need the buffer to be done because were releasing it right after
		pShaderContext.Finish(true);*/
		return S_OK;
	}

	HRESULT D3D12Engine::CopySample(ID3D12Resource* resource)
	{
		
		return S_OK;
	}

	void D3D12Engine::ClearBackBuffer(GraphicsContext& pVideoContext, CRect windowRect)
	{

	}

	void D3D12Engine::Downscale(GraphicsContext& Context, CRect srcRect, CRect destRect, bool sw, std::wstring scaler)
	{
		
	}

	void D3D12Engine::Upscale(GraphicsContext& Context, CRect srcRect, CRect destRect,bool sw,std::wstring scaler)
	{
#if 0
		if (!sw)
			DrawPlanes(Context, m_pVideoOutputResourcePreScale);

		bool res;
		if (m_pCurrentUpScaler && m_pCurrentUpScaler->GetScalerName() != scaler)
		{
			UNLOAD_SCALER(m_pCurrentUpScaler)
		}

		if (!m_pCurrentUpScaler)
		{
			m_pCurrentUpScaler = new CD3D12DynamicScaler(scaler, &res);
			if (!res)
			{
				UNLOAD_SCALER(m_pCurrentUpScaler)
				DLog(L"ERROR loading scaler {}", scaler);
				return;
			}

			m_pCurrentUpScaler->Init(DXGI_FORMAT_R8G8B8A8_UNORM,srcRect,destRect);

		}

		m_pCurrentUpScaler->Render(Context, destRect, m_pVideoOutputResource, m_pVideoOutputResourcePreScale);
#endif
	}

	void D3D12Engine::Noscale(GraphicsContext& Context,CRect dstRect,bool sw)
	{

	}

	void D3D12Engine::DrawPlanes(GraphicsContext& Context, ColorBuffer& output, CRect dstRect)
	{

	  
	}

	void D3D12Engine::PresentBackBuffer(GraphicsContext& Context,ColorBuffer& source)
	{
		
		Context.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
		ImageScaling::RenderToBackBuffer(Context, SwapChainBufferColor[p_CurrentBuffer], source);

		Context.SetViewportAndScissor(g_windowRect.left, g_windowRect.top, g_windowRect.Width(), g_windowRect.Height());
		//Context.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
		//ImageScaling::PreparePresentSDR(Context, SwapChainBufferColor[p_CurrentBuffer], m_pVideoOutputResourcePreScale, g_videoRect);
		Context.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_PRESENT);
	}

	void D3D12Engine::PresentBackBuffer(GraphicsContext& Context)
	{
		assert(0);
		//Context.TransitionResource(m_pVideoOutputResource, D3D12_RESOURCE_STATE_RENDER_TARGET);
		Context.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
		//ImageScaling::RenderToBackBuffer(Context, SwapChainBufferColor[p_CurrentBuffer], m_pVideoOutputResource);
		
		Context.SetViewportAndScissor(g_windowRect.left, g_windowRect.top, g_windowRect.Width(), g_windowRect.Height());
		//Context.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
		//ImageScaling::PreparePresentSDR(Context, SwapChainBufferColor[p_CurrentBuffer], m_pVideoOutputResourcePreScale, g_videoRect);
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
		p_CurrentBuffer = (p_CurrentBuffer + 1) % g_CurrentBufferCount;
	
	}

}/*D3D12Engine End*/