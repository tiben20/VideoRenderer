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

#include <d3d12.h>

#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include "dxva2api.h"
#include "helper.h"
#include "ImageScaling.h"
#include "Colorbuffer.h"
#include "descriptorheap.h"
#include "commandcontext.h"
#include "dxgi.h"
#include "OptionsFile.h"
#include "D3D12Util/Scaler.h"
class CommandListManager;
class DescriptorAllocator;
class ContextManager;
class ColorBuffer;
class CD3D12DynamicScaler;
class RootSignature;
namespace D3D12Engine
{

	/* Graphic pipelines */
	extern ID3D12Device* g_Device;
	extern CommandListManager g_CommandManager;
	extern ContextManager g_ContextManager;
	extern DescriptorAllocator g_DescriptorAllocator[];
	extern D3D_FEATURE_LEVEL g_D3DFeatureLevel;
	extern DWORD g_VendorId;
	extern CRect g_videoRect;
	extern CRect g_windowRect;
	extern CRect g_renderRect;
	extern CD3D12Options* g_Options;
	extern CD3D12DynamicScaler* m_pCurrentUpScaler;
	extern CD3D12DynamicScaler* m_pCurrentDownScaler;
	extern CD3D12DynamicScaler* m_pCurrentChromaScaler;
	extern CD3D12DynamicScaler* m_pCurrentImageDoubler;
	extern RootSignature g_RootScalers;
	extern int g_CurrentBufferCount;
	inline D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count = 1)
	{
		return g_DescriptorAllocator[Type].Allocate(Count);
	}

	HRESULT CreateDevice();
	HRESULT InitSwapChain(CRect windowRect);
	HRESULT ResetSwapChain(CRect windowrect, int presentSurfaceCount);
	HRESULT InitDXGIFactory();
	void CheckSwapChain(CRect windowrect);

	void SetShaderConvertColorParams(DXVA2_ExtendedFormat srcExFmt, FmtConvParams_t m_srcParams, DXVA2_ProcAmpValues m_DXVA2ProcAmpValues);
	bool ProcessHDR(DXVA2_ExtendedFormat srcExFmt, FmtConvParams_t m_srcParams);


	void SetVideoRect(CRect rect); 
	void SetWindowRect(CRect rect);
	void SetRenderRect(CRect rect);

	std::wstring GetAdapterDescription();
	DXGI_FORMAT GetSwapChainFormat();
	DXGI_FORMAT GetInternalFormat();
	D3D12_RESOURCE_DESC GetSwapChainResourceDesc();
	IDXGIFactory1* GetDXGIFactory();
	bool HdrPassthroughSupport();
	ColorBuffer GetCurrentBackBuffer();

	void  ReleaseEngine();
	void ReleaseSwapChain();

	HRESULT RenderAlphaBitmap(GraphicsContext& Context, Texture resource, D3D12_VIEWPORT vp, RECT alphaBitmapRectSrc);
	HRESULT RenderSubPic(GraphicsContext& Context, ColorBuffer resource, CRect srcRect, UINT srcW, UINT srcH);
	
	HRESULT CopySampleShaderPassSW(Texture& buf1, Texture& buf2, CRect dstRect);
	HRESULT CopySample(ID3D12Resource* resource);
	
	void ClearBackBuffer(GraphicsContext& Context, CRect windowRect );
	
	void DrawPlanes(GraphicsContext& Context, ColorBuffer& output, CRect dstRect = CRect());
	void Noscale(GraphicsContext& Context, CRect dstRect, bool sw);
	void Downscale(GraphicsContext& Context, CRect srcRect, CRect destRect, bool sw, std::wstring scaler);
	void Upscale(GraphicsContext& Context, CRect srcRect, CRect destRect, bool sw, std::wstring scaler);
	void PostShaders(GraphicsContext& Context, CRect srcRect, CRect destRect);
	void InitPostShaders(std::vector<CScalerOption*> options, CRect srcRect, CRect destRect);

	ColorBuffer GetPreScale();
	void PresentBackBuffer(GraphicsContext& Context);
	void WaitForVBlank();
	void Present();

	/* Swapchain */
	extern HWND g_hWnd;

	void InitializeRenderingBuffers(uint32_t NativeWidth, uint32_t NativeHeight);

	void DestroyRenderingBuffers();
}
