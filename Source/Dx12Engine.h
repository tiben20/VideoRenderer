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
#include "d3d12util/ImageScaling.h"

#include "d3d12util/descriptorheap.h"
#include "d3d12util/commandcontext.h"
class CommandListManager;
class DescriptorAllocator;
class ContextManager;
class ColorBuffer;


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
	/* */
	inline D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count = 1)
	{
		return g_DescriptorAllocator[Type].Allocate(Count);
	}

	HRESULT CreateDevice();
	HRESULT InitSwapChain(CRect windowRect);
	HRESULT ResetSwapChain(CRect windowrect);
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
	bool HdrPassthroughSupport();

	void  ReleaseEngine();
	void ReleaseSwapChain();

	HRESULT CopySampleSW(TypedBuffer buf1, TypedBuffer buf2, D3D12_PLACED_SUBRESOURCE_FOOTPRINT layoutplane[2]);
	HRESULT CopySample(ID3D12Resource* resource);
	void ClearBackBuffer(CRect windowRect);
	
	void Upscale(GraphicsContext& Context, ImageScaling::eScalingFilter tech = ImageScaling::kLanczos, CRect destRect = CRect());
	void PresentBackBuffer(GraphicsContext& Context);
	void WaitForVBlank();
	void Present();

	/* Swapchain */
	extern HWND g_hWnd;

	extern ColorBuffer g_SceneColorBuffer;  // R11G11B10_FLOAT
	extern ColorBuffer g_SceneNormalBuffer; // R16G16B16A16_FLOAT
	extern ColorBuffer g_OverlayBuffer;     // R8G8B8A8_UNORM
	extern ColorBuffer g_HorizontalBuffer;  // For separable (bicubic) upsampling
	void InitializeRenderingBuffers(uint32_t NativeWidth, uint32_t NativeHeight);

	void DestroyRenderingBuffers();

}
