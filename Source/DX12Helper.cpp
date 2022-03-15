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
#include "DX12Helper.h"
#include <string>
#include "d3d12util/CommandListManager.h"
namespace D3D12Public
{
	ID3D12Device* g_Device = nullptr;
	CommandListManager g_CommandManager;
	ContextManager g_ContextManager;
	//RootSignature s_PresentRS;

	

	HWND g_hWnd = nullptr;
	DescriptorAllocator g_DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] =
	{
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV
	};

}


HRESULT CD3DInclude::Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
{
	HRESULT hr = D3D11_ERROR_FILE_NOT_FOUND;


	std::string fullpath;
	std::string filecontent;
	//if (IncludeType == D3D10_INCLUDE_SYSTEM) we dont use this one

	fullpath.append(m_assetDir);
	fullpath.append(pFileName);

	replace_all(fullpath, "/", "\\");

	//fullpath.push_back(filecontent.c_str());
	std::ifstream file(fullpath.c_str());
	std::string str;
	filecontent.clear();
	while (std::getline(file, str))
	{
		filecontent += str;
		filecontent.push_back('\n');
		// Process str
	}
	char* result = new char[filecontent.length()];
	std::memcpy(result, filecontent.data(), filecontent.size());

	*ppData = result;
	*pBytes = filecontent.length();
	hr = S_OK;
	file.close();
	if (FAILED(hr))
	DLog(L"Failed to include shader header.");
	return hr;

	}