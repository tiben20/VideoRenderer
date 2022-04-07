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

enum :int {
	TEXFMT_AUTOINT = 0,
	TEXFMT_8INT = 8,
	TEXFMT_10INT = 10,
	TEXFMT_16FLOAT = 16,
};

enum :int {
	CHROMA_Nearest = 0,
	CHROMA_Bilinear,
	CHROMA_CatmullRom,
	CHROMA_COUNT
};

enum :int {
	UPSCALE_Nearest = 0,
	UPSCALE_Mitchell,
	UPSCALE_CatmullRom,
	UPSCALE_Lanczos2,
	UPSCALE_Lanczos3,
	UPSCALE_COUNT
};

enum :int {
	DOWNSCALE_Box = 0,
	DOWNSCALE_Bilinear,
	DOWNSCALE_Hamming,
	DOWNSCALE_Bicubic,
	DOWNSCALE_BicubicSharp,
	DOWNSCALE_Lanczos,
	DOWNSCALE_COUNT
};

enum :int {
	SWAPEFFECT_Discard = 0,
	SWAPEFFECT_Flip,
	SWAPEFFECT_COUNT
};

enum :int {
	HDRTD_Off = 0,
	HDRTD_Fullscreen = 1,
	HDRTD_Always = 2
};

struct VPEnableFormats_t {
	bool bNV12;
	bool bP01x;
	bool bYUY2;
	bool bOther;
};


struct superresConfig_t {
	int iStrength;//0 to 5
	float fSharp; // 0 to 1.5
	int iFactor; //0,1,2,3 but in the player its 2 4 8 or 16
};
struct superxbrConfig_t{
	int iStrength;//0 to 5
	float fSharp; // 0 to 1.5
	int iFactor; //0,1,2,3 but in the player its 2 4 8 or 16
};
struct superresxbrConfig_t {
	int iXbrStrength;//0 to 5
	float fXbrSharp; // 0 to 1.5
	int iXbrFactor; //0,1,2,3 but in the player its 2 4 8 or 16
	int iResStrength;//0 to 5
	float fresSharp; // 0 to 1.5
	int iResFactor; //0,1,2,3 but in the player its 2 4 8 or 16
};
struct D3D12Settings_t {
	bool bUseD3D12;
	bool bForceD3D12;
	bool bLAVUseD3D12;
	int chromaUpsampling;
	int imageDownscaling;
	int imageUpscaling;
	int imageUpscalingDoubling;
	superxbrConfig_t xbrConfig;
	superresConfig_t resConfig;
	superresxbrConfig_t xbrresConfig;
};

struct Settings_t {
	bool bUseD3D11;
	bool bShowStats;
	int  iResizeStats;
	int  iTexFormat;
	D3D12Settings_t D3D12Settings;
	VPEnableFormats_t VPFmts;
	bool bDeintDouble;
	bool bVPScaling;
	int  iChromaScaling;
	int  iUpscaling;
	int  iDownscaling;
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
		iTexFormat                      = TEXFMT_AUTOINT;
		VPFmts.bNV12                    = true;
		VPFmts.bP01x                    = true;
		VPFmts.bYUY2                    = true;
		VPFmts.bOther                   = true;
		bDeintDouble                    = true;
		bVPScaling                      = true;
		iChromaScaling                  = CHROMA_Bilinear;
		iUpscaling                      = UPSCALE_CatmullRom;
		iDownscaling                    = DOWNSCALE_Hamming;
		bInterpolateAt50pct             = true;
		bUseDither                      = true;
		iSwapEffect                     = SWAPEFFECT_Discard;
		bExclusiveFS                    = false;
		bVBlankBeforePresent            = false;
		bReinitByDisplay                = false;
		if (IsWindows10OrGreater()) {
			bHdrPassthrough             = true;
			iHdrToggleDisplay           = HDRTD_Always;
		} else {
			bHdrPassthrough             = false;
			iHdrToggleDisplay           = HDRTD_Off;
		}
		bConvertToSdr                   = true;
		D3D12Settings.bForceD3D12 = false;
		D3D12Settings.bLAVUseD3D12 = false;
		D3D12Settings.chromaUpsampling = 0;
		D3D12Settings.imageDownscaling = 0;
		D3D12Settings.imageDownscaling = 0;
		D3D12Settings.imageUpscalingDoubling = 0;
	}
};

interface __declspec(uuid("1AB00F10-5F55-42AC-B53F-38649F11BE3E"))
IVideoRenderer : public IUnknown {
	STDMETHOD(GetVideoProcessorInfo) (std::wstring& str) PURE;
	STDMETHOD_(bool, GetActive()) PURE;

	STDMETHOD_(void, GetSettings(Settings_t& setings)) PURE;
	STDMETHOD_(void, SetSettings(const Settings_t& setings)) PURE;

	STDMETHOD(SaveSettings()) PURE;
};
