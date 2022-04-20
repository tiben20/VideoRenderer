/*
 *
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

#include "IVideoRenderer.h"
#include <map>
#include "OptionsFile.h"

// CD3D12SettingsPPage
enum ScalerType
{
	Upscaler = 0,
	Downscaler = 1,
	Chromaupscaler =2,
	ImageDouble = 3,
	PostShader = 4
};

static const char* s_scalertype[5] = {
	{"UPSCALER"   },
	{"DOWNSCALER"},
	{"CHROMAUPSCALER"},
	{"IMAGEDOUBLER"},
	{"POST"}
};

static const char* s_upscalername[9] = {
	{"bilinear"   },
	{"dxva2"      },
	{"cubic"      },
	{"lanczos"    },
	{"spline"     },
	{"jinc"       },
	{"superxbr"   },
	{"fxrcnnx"   },
	{"superresxbr"},
};
static float s_jincsinc[4] = {
	{0.82f},
	{0.00001f},
	{1.0f},
	{0.25f},
};
static float s_jincwindowsinc[4] = {
	{0.44f},
	{0.00001f},
	{1.0f},
	{0.01f},
};
static float s_jincstr[4] = {
	{0.5f},
	{0.25f},
	{1.0f},
	{0.1f},
};

static int s_factor[4] = {
	{2},
	{4},
	{8},
	{16},
};
struct ShaderConstantDesc;

class __declspec(uuid("465D2B19-DE41-4425-8666-FB7FF0DAF122"))
	CD3D12SettingsPPage : public CBasePropertyPage, public CWindow
{
	CComQIPtr<IVideoRenderer> m_pVideoRenderer;
	Settings_t m_SetsPP;

public:
	CD3D12SettingsPPage(LPUNKNOWN lpunk, HRESULT* phr);
	~CD3D12SettingsPPage();

private:
	
	HRESULT OnConnect(IUnknown* pUnknown) override;
	HRESULT OnDisconnect() override;
	HRESULT OnActivate() override;
	INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	HRESULT OnApplyChanges() override;
	
	void UpdateCurrentPostScaler();
	void UpdateCurrentScaler(std::vector<ShaderConstantDesc> shaderconst);

	void FillScalers(ScalerType scalertype);

	void SetControlConfig(HWND hwnd, int editidx, int slideridx, ShaderConstantDesc sconst);
	void SetShaderDescription(std::wstring file);
	void SetShaderOptions(std::wstring file);

	void UpdateScroll(int button, int staticbutton,int value);
	void SetDirty()
	{
		m_bDirty = TRUE;
		if (m_pPageSite)
		{
			m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
		}
	}
	
	CPoint         m_pDragPostScaler;
	CScalerOption* m_pCurrentScalerOption;
	ScalerType   m_pCurrentScalerType;
	std::wstring m_sCurrentUpScaler = L"";
	std::wstring m_sCurrentDownScaler = L"";
	std::wstring m_sCurrentChromaUpscaler = L"";
	std::wstring m_sCurrentImageDoubler = L"";
	std::vector<std::wstring> m_sCurrentPostScaler;
	std::vector<ShaderConstantDesc> m_pCurrentShaderConstants;
};
