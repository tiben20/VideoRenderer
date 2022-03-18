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
#include "d3d12util/descriptorheap.h"
#include "d3d12util/commandcontext.h"
class CommandListManager;
class DescriptorAllocator;
class ContextManager;
class ColorBuffer;
namespace D3D12Public
{
	extern ID3D12Device* g_Device;
	extern CommandListManager g_CommandManager;
	extern uint32_t g_DisplayWidth;
	extern uint32_t g_DisplayHeight;
	extern bool g_bEnableHDROutput;
	extern ContextManager g_ContextManager;

	extern D3D_FEATURE_LEVEL g_D3DFeatureLevel;
	extern bool g_bTypedUAVLoadSupport_R11G11B10_FLOAT;
	extern bool g_bTypedUAVLoadSupport_R16G16B16A16_FLOAT;

	extern DescriptorAllocator g_DescriptorAllocator[];
	inline D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count = 1)
	{
		return g_DescriptorAllocator[Type].Allocate(Count);
	}
	extern HWND g_hWnd;

	extern ColorBuffer g_DisplayPlane[3];//3 swap chap buffer
	//RootSignature s_PresentRS;
}

class CD3DInclude : public ID3DInclude
{
public:
	CD3DInclude(const char* assetDir) : m_assetDir(assetDir) {}
	~CD3DInclude() {}

	std::size_t replace_all(std::string& inout, std::string_view what, std::string_view with)
	{
		std::size_t count{};
		for (std::string::size_type pos{};
			inout.npos != (pos = inout.find(what.data(), pos, what.length()));
			pos += with.length(), ++count) {
			inout.replace(pos, what.length(), with.data(), with.length());
		}
		return count;
	}

	HRESULT Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes);
	
	HRESULT Close(LPCVOID pData)
	{

		delete[]((char*)pData);
		return S_OK;
	}
private:
	std::string m_assetDir;
};