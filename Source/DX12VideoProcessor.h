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

#include <DXGI1_2.h>
#include <dxva2api.h>
#include <dxgi1_5.h>
#include <strmif.h>
#include <map>
#include "IVideoRenderer.h"
#include "dx12helper.h"
#include "DX9Device.h"
#include "VideoProcessor.h"

#define TEST_SHADER 0

class CVideoRendererInputPin;

class CDX12VideoProcessor
	: public CVideoProcessor
	, public CDX9Device
{
private:
	friend class CVideoRendererInputPin;

	// Direct3D 11
	CComPtr<ID3D12Device1>        m_pDevice;

	DXGI_SWAP_EFFECT              m_UsedSwapEffect = DXGI_SWAP_EFFECT_DISCARD;

public:
	CDX12VideoProcessor(CMpcVideoRenderer* pFilter, const Settings_t& config, HRESULT& hr);
	~CDX12VideoProcessor() override;

	bool Initialized();
private:
	void ReleaseVP();
	void ReleaseDevice();
};
