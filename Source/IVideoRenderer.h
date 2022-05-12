/*
 * (C) 2018-2021 see Authors.txt
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

#include <dxva2api.h>

struct D3D12Settings_t {
	bool bUseD3D12;
	bool bForceD3D12;
	bool bLAVUseD3D12;
};

struct Settings_t {
	bool bUseD3D11;
	bool bShowStats;
	int  iResizeStats;
	int  iTexFormat;
	D3D12Settings_t D3D12Settings;
	bool bDeintDouble;
	bool bInterpolateAt50pct;
	bool bUseDither;
	int  iSwapEffect;
	bool bExclusiveFS;
	bool bVBlankBeforePresent;
	bool bReinitByDisplay;
	bool bHdrPassthrough;
	int  iHdrToggleDisplay;
	bool bConvertToSdr;

	Settings_t() {
		SetDefault();
	}

	void SetDefault() {
		if (IsWindows8OrGreater()) {
			bUseD3D11                     = true;
			D3D12Settings.bUseD3D12       = false;
		} else {
			bUseD3D11                     = false;
			D3D12Settings.bUseD3D12       = false;
		}
		bShowStats                      = false;
		iResizeStats                    = 0;
		


		bDeintDouble                    = true;
		bInterpolateAt50pct             = true;
		bUseDither                      = true;
		
		bExclusiveFS                    = false;
		bVBlankBeforePresent            = false;
		bReinitByDisplay                = false;

		bConvertToSdr                   = true;
		D3D12Settings.bForceD3D12 = false;
		D3D12Settings.bLAVUseD3D12 = false;
	}
};
//uuid changed still need change the interface settings not planning on the same way of using settings
interface __declspec(uuid("B58AEDA7-29E5-4FCD-AC26-491E697F5DC7"))
IVideoRenderer : public IUnknown {
	STDMETHOD(GetVideoProcessorInfo) (std::wstring& str) PURE;
	STDMETHOD_(bool, GetActive()) PURE;

	//STDMETHOD_(void, GetSettings(Settings_t& setings)) PURE;
	//STDMETHOD_(void, SetSettings(const Settings_t& setings)) PURE;

	STDMETHOD(SaveSettings()) PURE;
};
