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
#include <d3d12video.h>
#include "d3dx12.h"
#include "texture.h"
// D3D11 Video Processor
class CD3D12VP
{
private:
	CComPtr<ID3D12VideoDevice> m_pVideoDevice;
	CComPtr<ID3D12VideoProcessor> m_pVideoProcessor;
	CRect m_srcRect;
	Texture m_pInputTextures;
	Texture m_pOutputTextures;
	DXGI_FORMAT m_pInputFormat;
	DXGI_FORMAT m_pOutputFormat;
public:
	HRESULT InitVideoDevice(ID3D12Device *pDevice);
	void ReleaseVideoDevice();

	HRESULT InitVideoProcessor(const DXGI_FORMAT inputFmt, const UINT width, const UINT height, const bool interlaced, DXGI_FORMAT& outputFmt);
	void ReleaseVideoProcessor();

	HRESULT InitInputTextures();

	bool IsVideoDeviceOk() { return (m_pVideoDevice != nullptr); }
	HRESULT Process(ID3D12Resource* target, ID3D12Resource* sub);

};
