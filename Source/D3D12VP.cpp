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
#include <cmath>
#include "Helper.h"
#include "DX12Engine.h"

#include "D3D12VP.h"

#define ENABLE_FUTUREFRAMES 0



// CD3D12VP

HRESULT CD3D12VP::InitVideoDevice(ID3D12Device *pDevice)
{
	HRESULT hr = pDevice->QueryInterface(IID_PPV_ARGS(&m_pVideoDevice));
	if (FAILED(hr)) {
		DLog(L"CD3D12VP::InitVideoDevice() : QueryInterface(ID3D12VideoDevice) failed with error {}", HR2Str(hr));
		ReleaseVideoDevice();
		return hr;
	}

	return hr;
}

void CD3D12VP::ReleaseVideoDevice()
{
	ReleaseVideoProcessor();

	m_pVideoDevice.Release();
}

HRESULT CD3D12VP::InitVideoProcessor(const DXGI_FORMAT inputFmt, const UINT width, const UINT height, const bool interlaced, DXGI_FORMAT& outputFmt)
{
	ReleaseVideoProcessor();
	HRESULT hr = S_OK;

	return hr;
}

void CD3D12VP::ReleaseVideoProcessor()
{
	
	
}

HRESULT CD3D12VP::InitInputTextures(ID3D12Device* pDevice)
{

	HRESULT hr = E_NOT_VALID_STATE;

	return hr;
}
