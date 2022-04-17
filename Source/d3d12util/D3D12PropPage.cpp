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

#include "stdafx.h"
#include "resource.h"
#include "Helper.h"
#include "DisplayConfig.h"
#include "D3D12PropPage.h"
#include "PropPage.h"
#include "Dx12Engine.h"
#include <iostream>
#include <filesystem>
enum eD3D12Upscalers
{
	bilinear = 0,
	d3d12 = 1,
	cubic = 2,
	lanczos = 3,
	spline = 4,
	jinc = 5,
	superxbr =6,
	fxrcnnx = 7,
	superresxbr = 8

};


inline void SetRadioValue(HWND hWnd, int nIDComboBox, UINT msg, WPARAM wParam, LPARAM lParam)
{
	SendDlgItemMessageW(hWnd, nIDComboBox, msg, wParam, lParam);
}

inline LRESULT GetRadioValue(HWND hWnd, int nIDComboBox)
{
	return SendDlgItemMessage(hWnd, nIDComboBox, BM_GETCHECK, 0, 0);
}

inline int GetRangeMin(HWND hWnd,int nID )
{
	return (int)SendDlgItemMessageW(hWnd,nID, TBM_GETRANGEMIN, 0, 0L);
}

inline int GetPos(HWND hWnd, int nID)
{
	return (int)SendDlgItemMessageW(hWnd, nID, TBM_GETPOS, 0, 0L);
}

inline void SetRangeMinMax(HWND hWnd, int nID, int nMin, int nMax, BOOL bRedraw = FALSE)
{
	SendDlgItemMessageW(hWnd,nID, TBM_SETRANGEMIN, bRedraw, nMin);
	SendDlgItemMessageW(hWnd, nID, TBM_SETRANGEMAX, bRedraw, nMax);
}

inline void SetRange(HWND hWnd, int nID, int nMin, int nMax, BOOL bRedraw = TRUE)
{
	SendDlgItemMessageW(hWnd,nID, TBM_SETRANGE, bRedraw, MAKELPARAM(nMin, nMax));
}

inline int GetSelStart(HWND hWnd, int nID)
{
	return (int)SendDlgItemMessageW(hWnd,nID, TBM_GETSELSTART, 0, 0L);
}

inline int GetSelEnd(HWND hWnd, int nID)
{
	return (int)SendDlgItemMessageW(hWnd, nID, TBM_GETSELEND, 0, 0L);
}

inline void SetSelStart(HWND hWnd, int nID, int nMin, BOOL bRedraw = FALSE)
{
	SendDlgItemMessageW(hWnd,nID, TBM_SETSELSTART, bRedraw, (LPARAM)nMin);
}

inline void SetSelEnd(HWND hWnd, int nID, int nMax, BOOL bRedraw = FALSE)
{
	SendDlgItemMessageW(hWnd, nID, TBM_SETSELEND, bRedraw, (LPARAM)nMax);
}

inline void GetSliderSelection(HWND hWnd, int nID,int& nMin, int& nMax)
{
	nMin = GetSelStart(hWnd,nID);
	nMax = GetSelEnd(hWnd, nID);
}

inline void SetSliderSelection(HWND hWnd, int nID, int nMin, int nMax, BOOL bRedraw = TRUE)
{
	SetSelStart(hWnd,nID,nMin, FALSE);
	SetSelEnd(hWnd, nID, nMax, bRedraw);
}

inline void SetPos(HWND hWnd, int nID, int nPos)
{
	SendDlgItemMessageW(hWnd, nID, TBM_SETPOS, TRUE, nPos);
}

inline void SetEditModify(HWND hWnd, int nID, bool bModified = TRUE)
{
	SendDlgItemMessageW(hWnd, nID, EM_SETMODIFY, bModified, 0L);
}

inline void ResetContent(HWND hWnd, int nID)
{
	SendDlgItemMessageW(hWnd, nID, CB_RESETCONTENT, 0, 0L);
}

inline std::wstring GetEditText(HWND hWnd, int nID)
{

	TCHAR str[128];
	LRESULT res = SendDlgItemMessageW(hWnd, nID, WM_GETTEXT, 128, (LPARAM)&str);
	 return str;
}

template<typename T> inline void SetEditText(HWND hWnd, int nID, T value)
{
	
	std::wstring str = fmt::format(L"{}", value);
	SendDlgItemMessageW(hWnd, nID, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(str.c_str()));
}

inline int GetSliderID(HWND hWnd)
{
	return GetDlgCtrlID(hWnd);
}

inline void ListboxClear(HWND hWnd, int nIDListBox)
{
	SendDlgItemMessageW(hWnd, nIDListBox, LB_RESETCONTENT, 0, 0);
}


inline LRESULT ListboxAddString(HWND hWnd, int nIDListBox, LPCWSTR str)
{
	LRESULT lValue = SendDlgItemMessageW(hWnd, nIDListBox, LB_ADDSTRING, 0, (LPARAM)str);
	return lValue;
}

inline std::wstring ListboxGetCurrentText(HWND hWnd, int nIDListBox)
{
	TCHAR res[200];
	LRESULT lIndex = SendDlgItemMessageW(hWnd, nIDListBox, LB_GETCURSEL, (WPARAM)0, (LPARAM)0);
	SendDlgItemMessageW(hWnd, nIDListBox, LB_GETTEXT, (WPARAM)lIndex, (LPARAM)&res);

	return res;
}

inline void ListboxSetSelected(HWND hWnd, int nIDListBox,int index)
{
	LRESULT res = SendDlgItemMessageW(hWnd, nIDListBox, LB_SETCURSEL, (WPARAM)index, (LPARAM)0);
}

// CD3D12SettingsPPage

CD3D12SettingsPPage::CD3D12SettingsPPage(LPUNKNOWN lpunk, HRESULT* phr) :
	CBasePropertyPage(L"D3D12Prop", lpunk, IDD_D3D12PROPPAGE, IDS_D3D12PROP_TITLE)
{
	DLog(L"CD3D12SettingsPPage()");
	
}

CD3D12SettingsPPage::~CD3D12SettingsPPage()
{
	DLog(L"~CD3D12SettingsPPage()");

}

HRESULT CD3D12SettingsPPage::OnConnect(IUnknown *pUnk)
{
	if (pUnk == nullptr) return E_POINTER;

	m_pVideoRenderer = pUnk;
	if (!m_pVideoRenderer) {
		return E_NOINTERFACE;
	}

	return S_OK;
}

HRESULT CD3D12SettingsPPage::OnDisconnect()
{
	if (m_pVideoRenderer == nullptr) {
		return E_UNEXPECTED;
	}

	m_pVideoRenderer.Release();

	return S_OK;
}

HRESULT CD3D12SettingsPPage::OnActivate()
{
	// set m_hWnd for CWindow
	m_hWnd = m_hwnd;
	RECT rctlist;
	rctlist.left = 10;
	rctlist.top = 10;
	rctlist.right = 200;
	rctlist.bottom = 400;

	m_pVideoRenderer->GetSettings(m_SetsPP);

	if (!D3D12Engine::g_D3D12Options)
		D3D12Engine::g_D3D12Options = new CD3D12Options();
	m_sCurrentUpScaler = D3D12Engine::g_D3D12Options->GetCurrentUpscaler();
	m_sCurrentChromaUpscaler = D3D12Engine::g_D3D12Options->GetCurrentChromaUpscaler();
	m_sCurrentDownScaler = D3D12Engine::g_D3D12Options->GetCurrentDownscaler();
	m_sCurrentImageDoubler = D3D12Engine::g_D3D12Options->GetCurrentImageDoubler();
	
	//todo postscaler m_sCurrentDownScaler = D3D12Engine::g_D3D12Options->get();
	SetRadioValue(m_hWnd, IDC_RADIO1, BM_SETCHECK, BST_CHECKED, 0);
	FillScalers(ScalerType::Upscaler);
	SetShaderOptions(m_sCurrentUpScaler);
#if 0
	int i;
	for (i = 0; i < 9; i++)
		SetRadioValue(m_hWnd, IDC_RADIO_CHROMAUP1 + i, BM_SETCHECK, (m_iCurrentChromaUpscaler == (i)), 0);
	//for (i = 0; i < 5; i++)
		//SetRadioValue(m_hWnd, IDC_RADIO_DOUBLING1 + i, BM_SETCHECK, (m_SetsPP.D3D12Settings.imageUpscalingDoubling == (i)), 0);
	for (i = 0; i < 9; i++)
		SetRadioValue(m_hWnd, IDC_RADIO_UPSCALING1 + i, BM_SETCHECK, (m_iCurrentUpScaler == (i)), 0);
	
	for (i = 0; i < 6; i++)
		SetRadioValue(m_hWnd, IDC_RADIO_DOWNSCALING1 + i, BM_SETCHECK, (m_iCurrentDownScaler == (i)), 0);

	CheckDlgButton(IDC_CHECK13, m_SetsPP.D3D12Settings.bUseD3D12 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK17, m_SetsPP.D3D12Settings.bForceD3D12 ? BST_CHECKED : BST_UNCHECKED);
	
	/*SetRangeMinMax(m_hWnd, IDC_SLIDER1, 0, 5);//str default 1
	SetRangeMinMax(m_hWnd, IDC_SLIDER2, 0, 15);//sharp 0 to 1.5  default 1
	SetRangeMinMax(m_hWnd, IDC_SLIDER3, 0, 3);//factor 2, 4 , 8 , 16 default 2*/
	/*SetPos(m_hWnd, IDC_SLIDER1, (m_SetsPP.D3D12Settings.xbrConfig.iStrength));
	SetEditText(m_hWnd, IDC_EDIT1, m_SetsPP.D3D12Settings.xbrConfig.iStrength);
	SetPos(m_hWnd, IDC_SLIDER2, (m_SetsPP.D3D12Settings.xbrConfig.fSharp * 10));
	SetEditText(m_hWnd, IDC_EDIT2, m_SetsPP.D3D12Settings.xbrConfig.fSharp);
	SetPos(m_hWnd, IDC_SLIDER3, (m_SetsPP.D3D12Settings.xbrConfig.iFactor));
	SetEditText(m_hWnd, IDC_EDIT3, m_SetsPP.D3D12Settings.xbrConfig.iStrength);*/
	
#endif
	UpdateCurrentScaler();
	return S_OK;
}

void CD3D12SettingsPPage::UpdateCurrentScaler()
{
#if 0
	SetEditText(m_hWnd, IDC_STATIC_XBR_STR, L"");
	SetEditText(m_hWnd, IDC_STATIC_XBR_STR2, L"");
	SetEditText(m_hWnd, IDC_STATIC_XBR_STR3, L"");
	SetEditText(m_hWnd, IDC_STATIC_XBR_STR4, L"");
	SetEditText(m_hWnd, IDC_STATIC_XBR_STR5, L"");
	SetEditText(m_hWnd, IDC_STATIC_XBR_STR6, L"");
	GetDlgItem(IDC_SLIDER1).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SLIDER2).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SLIDER3).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SLIDER4).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SLIDER5).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SLIDER6).ShowWindow(SW_HIDE);
	//GetDlgItem(IDC_COMBO_OPTIONS).ShowWindow(SW_HIDE);
	
	GetDlgItem(IDC_EDIT1).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT2).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT3).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT4).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT5).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT6).ShowWindow(SW_HIDE);
	ResetContent(m_hWnd, IDC_COMBO_OPTIONS);
	CScalerOption* opt;
	eD3D12Upscalers currentupscaler = (eD3D12Upscalers)D3D12Engine::g_D3D12Options->GetCurrentUpscaler();
	//bilinear
	if (currentupscaler == eD3D12Upscalers::bilinear)
	{
		
	}
	//DXVA2
	if (currentupscaler == eD3D12Upscalers::d3d12)
	{
		
	}
	//Bicubuic
	if (currentupscaler == eD3D12Upscalers::cubic)
	{
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Mitchel-Netravali", 0);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Bicubic 50 / Catmull-Rom", 1);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Bicubic 60", 2);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Bicubic 75", 3);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Bicubic 100", 4);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Bicubic 125", 5);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Bicubic 150", 6);
		opt = D3D12Engine::g_D3D12Options->GetScaler("cubic");
		int bicubicmode = opt->GetInt("bicubic");
		SendDlgItemMessageW(IDC_COMBO_OPTIONS, CB_SETCURSEL, bicubicmode,0);
	}
	//lanczso
	if (currentupscaler == eD3D12Upscalers::lanczos)
	{
		opt = D3D12Engine::g_D3D12Options->GetScaler("lanczos");
		int taps = opt->GetInt("lanczostype");
		
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Lanczsos2", 2);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Lanczsos3", 3);
		SendDlgItemMessageW(IDC_COMBO_OPTIONS, CB_SETCURSEL, taps-2, 0);
	}
	//Spline
	if (currentupscaler == eD3D12Upscalers::spline)
	{
		opt = D3D12Engine::g_D3D12Options->GetScaler("spline");
		int taps = opt->GetInt("splinetype");
		
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Mitchell-Netravali", 0);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Catmull-Rom", 1);
		SendDlgItemMessageW(IDC_COMBO_OPTIONS, CB_SETCURSEL, taps, 0);
	}
	//Jinc
	if (currentupscaler == eD3D12Upscalers::jinc)
	{
		opt = D3D12Engine::g_D3D12Options->GetScaler("jinc");
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR, L"Window Sinc");
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR2, L"Sinc");
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR3, L"Anti-ringing Strength");
		GetDlgItem(IDC_SLIDER1).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SLIDER2).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SLIDER3).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT1).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT2).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT3).ShowWindow(SW_SHOW);
		SetRangeMinMax(m_hWnd, IDC_SLIDER1, 0, 3);//0.44 0.0 1.0 0.01
		SetRangeMinMax(m_hWnd, IDC_SLIDER2, 0, 3);//0.82 0.0 1.0 0.01
		SetRangeMinMax(m_hWnd, IDC_SLIDER3, 0, 3);//0.5 0.0 1.0 0.1
		SetPos(m_hWnd, IDC_SLIDER1, opt->GetInt("windowsinc"));
		SetPos(m_hWnd, IDC_SLIDER2, opt->GetInt("sinc"));
		SetPos(m_hWnd, IDC_SLIDER3, opt->GetInt("str"));
		SetEditText(m_hWnd, IDC_EDIT1, opt->GetInt("windowsinc"));
		SetEditText(m_hWnd, IDC_EDIT2, opt->GetInt("sinc"));
		SetEditText(m_hWnd, IDC_EDIT3, opt->GetInt("str"));
	}
	//super xbr
	if (currentupscaler == eD3D12Upscalers::superxbr)
	{
		opt = D3D12Engine::g_D3D12Options->GetScaler("superxbr");
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR, L"Strength");
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR2, L"Sharp");
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR3, L"Factor");
		GetDlgItem(IDC_SLIDER1).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SLIDER2).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SLIDER3).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT1).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT2).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT3).ShowWindow(SW_SHOW);
		SetRangeMinMax(m_hWnd, IDC_SLIDER1, 0, 5);//str default 1
		SetRangeMinMax(m_hWnd, IDC_SLIDER2, 0, 15);//sharp 0 to 1.5  default 1
		SetRangeMinMax(m_hWnd, IDC_SLIDER3, 0, 3);//factor 2, 4 , 8 , 16 default 2
		SetPos(m_hWnd, IDC_SLIDER1, opt->GetInt("strength"));
		SetPos(m_hWnd, IDC_SLIDER2, (opt->GetFloat("sharp")*10));
		SetPos(m_hWnd, IDC_SLIDER3, opt->GetInt("factor"));
		SetEditText(m_hWnd, IDC_EDIT1, opt->GetInt("strength"));
		SetEditText(m_hWnd, IDC_EDIT2, (opt->GetFloat("sharp")));
		SetEditText(m_hWnd, IDC_EDIT3, s_factor[opt->GetInt("factor")]);
		

	}
	//fxrcnnx
	if (currentupscaler == eD3D12Upscalers::fxrcnnx)
	{
		opt = D3D12Engine::g_D3D12Options->GetScaler("fxrcnnx");
		SetRangeMinMax(m_hWnd, IDC_SLIDER1, 1, 5);//passes
		SetRangeMinMax(m_hWnd, IDC_SLIDER2, 0, 10);//float 0 to 1
		SetRangeMinMax(m_hWnd, IDC_SLIDER3, 0, 10);//float 0 to 1
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR, L"Passes");
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR2, L"Strength");
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR3, L"Smoothness");
		GetDlgItem(IDC_SLIDER1).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SLIDER2).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SLIDER3).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT1).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT2).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT3).ShowWindow(SW_SHOW);
		SetPos(m_hWnd, IDC_SLIDER1, opt->GetInt("passes"));
		SetPos(m_hWnd, IDC_SLIDER2, (opt->GetFloat("strength") * 10));
		SetPos(m_hWnd, IDC_SLIDER3, opt->GetFloat("smoothness")* 10 );
		SetEditText(m_hWnd, IDC_EDIT1, opt->GetInt("passes"));
		SetEditText(m_hWnd, IDC_EDIT2, (opt->GetFloat("strength") * 10));
		SetEditText(m_hWnd, IDC_EDIT3, opt->GetFloat("smoothness") * 10);
		
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"spline", 0);
		//only one option will todo add more
		SendDlgItemMessageW(IDC_COMBO_OPTIONS, CB_SETCURSEL, 0, 0);
		//dither tools
		//planar upscale
	}
	//Super res + xbr
	if (currentupscaler == eD3D12Upscalers::superresxbr)
	{
		opt = D3D12Engine::g_D3D12Options->GetScaler("superresxbr");
		SetRangeMinMax(m_hWnd, IDC_SLIDER1, 0, 5);//str default 1
		SetRangeMinMax(m_hWnd, IDC_SLIDER2, 0, 15);//sharp 0 to 1.5  default 1
		SetRangeMinMax(m_hWnd, IDC_SLIDER3, 0, 3);//factor 2, 4 , 8 , 16 default 2
		SetRangeMinMax(m_hWnd, IDC_SLIDER4, 1, 5); //passes
		SetRangeMinMax(m_hWnd, IDC_SLIDER5, 0, 10);//float 0 to 1
		SetRangeMinMax(m_hWnd, IDC_SLIDER6, 0, 10);//float 0 to 1
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR, L"Xbr Strength");
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR2, L"Xbr Sharp");
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR3, L"Xbr Factor");
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR4, L"Res passes");
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR5, L"Res Strength");
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR6, L"Res Smoothness");
		GetDlgItem(IDC_SLIDER1).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SLIDER2).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SLIDER3).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SLIDER4).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SLIDER5).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SLIDER6).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT1).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT2).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT3).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT4).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT5).ShowWindow(SW_SHOW);
		GetDlgItem(IDC_EDIT6).ShowWindow(SW_SHOW);
		SetPos(m_hWnd, IDC_SLIDER1, opt->GetInt("xbrstrength"));
		SetPos(m_hWnd, IDC_SLIDER2, (opt->GetFloat("xbrsharp") * 10));
		SetPos(m_hWnd, IDC_SLIDER3, opt->GetInt("factor"));
		SetPos(m_hWnd, IDC_SLIDER4, opt->GetInt("respasses"));
		SetPos(m_hWnd, IDC_SLIDER5, (opt->GetFloat("resstrength") * 10));
		SetPos(m_hWnd, IDC_SLIDER6, opt->GetFloat("ressmoothness") * 10);
		SetEditText(m_hWnd, IDC_EDIT1, opt->GetInt("xbrstrength"));
		SetEditText(m_hWnd, IDC_EDIT2, (opt->GetFloat("xbrsharp")));
		SetEditText(m_hWnd, IDC_EDIT3, opt->GetInt("factor"));
		SetEditText(m_hWnd, IDC_EDIT4, opt->GetInt("respasses"));
		SetEditText(m_hWnd, IDC_EDIT5, (opt->GetFloat("resstrength")));
		SetEditText(m_hWnd, IDC_EDIT6, opt->GetFloat("ressmoothness"));

		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"spline", 0);
		SendDlgItemMessageW(IDC_COMBO_OPTIONS, CB_SETCURSEL, 0, 0);
	}
#endif
}

std::string GetOptionName(std::wstring inp)
{
	if (inp == L"Taps")
		return "taps";
	if (inp == L"Bicubic")
		return "bicubic";
	if (inp == L"Strength")
		return "strength";
	if (inp == L"Sharp")
		return "sharp";
	if (inp == L"Passes")
		return "passes";
	if (inp == L"Smoothness")
		return "smoothness";
	if (inp == L"Xbr Strength")
		return "xbrstrength";
	if (inp == L"Xbr Sharp")
		return "xbrsharp";
	if (inp == L"Res passes")
		return "respasses";
	if (inp == L"Res Strength")
		return "resstrength";
	if (inp == L"Res Smoothness")
		return "ressmoothness";
	if (inp == L"Window Sinc")
		return "windowsinc";
	if (inp == L"Sinc")
		return "sinc";
	if (inp == L"Anti-ringing Strength")
		return "str";
	return "";
}


int GetFactorUp(int in)
{
	for (int i = 0; i < 4; i++)
	{
		if (s_factor[i] == in)
			return i;
	}
	return 0;
}

void CD3D12SettingsPPage::UpdateScroll(int index, int staticbutton, int value)
{
	//m_pCurrentShaderConstants.at(index).defaultValue;
	if (m_pCurrentShaderConstants.size() == 0)
		return;
	if (m_pCurrentShaderConstants.at(index).type == ShaderConstantType::Int)
	{
		SetEditText(m_hWnd, staticbutton, value);
		m_pCurrentShaderConstants.at(index).currentValue = value;
	}
	else
	{
		SetEditText(m_hWnd, staticbutton, (float)(((float)value) / 100));
		m_pCurrentShaderConstants.at(index).currentValue = (float)(((float)value) / 100);
	}
	if (D3D12Engine::m_pCurrentUpScaler)
		D3D12Engine::m_pCurrentUpScaler->SetShaderConstants(m_pCurrentShaderConstants);
	
#if 0
	std::wstring strstatic = GetEditText(m_hWnd, button);
	SetDirty();
	CScalerOption* opt = D3D12Engine::g_D3D12Options->GetScaler(s_upscalername[m_iCurrentUpScaler]);

	if (strstatic.find(L"Factor") != std::wstring::npos)
	{
		
		int fact = s_factor[value];
		SetEditText(m_hWnd, staticbutton, fact);
		opt->SetInt("factor", value);
	}
	else if (strstatic == L"Xbr Strength" || strstatic == L"Res passes" || strstatic == L"Passes" || strstatic == L"Strength")
	{
		SetEditText(m_hWnd, staticbutton, value);
		opt->SetInt(GetOptionName(strstatic), value);
	}
	else if (strstatic == L"Window Sinc" || strstatic == L"Sinc" || strstatic == L"Anti-ringing Strength")
	{
		SetEditText(m_hWnd, staticbutton, value);
		opt->SetInt(GetOptionName(strstatic), value);
	}
	else
	{
		SetEditText(m_hWnd, staticbutton, float(float(value) / 10));
		opt->SetFloat(GetOptionName(strstatic), float(float(value) / 10));
	}
	D3D12Engine::g_D3D12Options->SetScaler(s_upscalername[m_iCurrentUpScaler], opt);
#endif
}

std::vector<std::wstring> get_all_files_names_within_folder(std::wstring folder)
{
	std::vector<std::wstring> names;
	std::wstring search_path = folder + L"/*.hlsl";
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// read all (real) files in current folder
			// , delete '!' read other 2 default folder . and ..
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				names.push_back(fd.cFileName);
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
	return names;
}

void CD3D12SettingsPPage::SetControlConfig(HWND hwnd, int editidx, int slideridx, ShaderConstantDesc sconst)
{
	GetDlgItem(slideridx).ShowWindow(SW_SHOW);
	GetDlgItem(editidx).ShowWindow(SW_SHOW);
	if (sconst.type == ShaderConstantType::Float)
	{
		//std::monostate var = std::get<std::monostate>(sconst.maxValue);
		if (sconst.maxValue.index() !=0)
			SetRangeMinMax(hwnd, slideridx, (std::get<float>(sconst.minValue) * 100), (std::get<float>(sconst.maxValue) * 100));
		else
		  SetRangeMinMax(hwnd, slideridx, (std::get<float>(sconst.minValue) * 100), 1000);
		SetPos(hwnd, slideridx, (std::get<float>(sconst.defaultValue) * 100));
		SetEditText(hwnd, editidx, std::get<float>(sconst.defaultValue));
	}
	else
	{
		if (sconst.maxValue.index() != 0)
			SetRangeMinMax(hwnd, slideridx, (std::get<int>(sconst.minValue)), (std::get<int>(sconst.maxValue)));
		else
			SetRangeMinMax(hwnd, slideridx, (std::get<int>(sconst.minValue)), 100);
		SetPos(hwnd, slideridx, (std::get<int>(sconst.defaultValue)));
		SetEditText(hwnd, editidx, std::get<int>(sconst.defaultValue));
	}

}
void CD3D12SettingsPPage::FillScalers(ScalerType scalertype)
{
	//TOFIX
	std::wstring appdata = _wgetenv(L"APPDATA");
	appdata.append(L"\\MPCVideoRenderer\\Shaders\\");
	std::filesystem::directory_iterator end_itr; // default construction yields past-the-end

	std::vector<std::wstring> files = get_all_files_names_within_folder(appdata);
	CShaderFileLoader* testshader = new CShaderFileLoader(L"");
	std::vector<std::wstring> shaders;
	std::string scl;
	m_pCurrentScalerType = scalertype;
	for (std::wstring fle : files)
	{
		scl = testshader->GetScalerType(appdata + fle);
		if (scl == "UPSCALER" && scalertype == ScalerType::Upscaler)
		{
			shaders.push_back(fle);
		}
		else if (scl == "DOWNSCALER" && scalertype == ScalerType::Downscaler)
		{
			shaders.push_back(fle);
		}
		else if (scl == "CHROMASCALER" && scalertype == ScalerType::Chromaupscaler)
		{
			shaders.push_back(fle);
		}
		else if (scl == "IMAGEDOUBLER" && scalertype == ScalerType::ImageDouble)
		{
			shaders.push_back(fle);
		}
		else if (scl == "POST" && scalertype == ScalerType::PostShader)
		{
			shaders.push_back(fle);
		}
	}

	LRESULT resindex = -1;
	LRESULT selindex = -1;
	std::wstring currentlyselected;
	switch (scalertype)
	{
	case Upscaler:
		currentlyselected = m_sCurrentUpScaler;
		break;
	case Downscaler:
		currentlyselected = m_sCurrentDownScaler;
		break;
	case Chromaupscaler:
		currentlyselected = m_sCurrentChromaUpscaler;
		break;
	case ImageDouble:
		currentlyselected = m_sCurrentImageDoubler;
		break;
	case PostShader:
		currentlyselected = m_sCurrentPostScaler;
		break;
	default:
		break;
	}
	
	for (std::wstring shdr : shaders)
	{
		resindex = ListboxAddString(m_hWnd, IDC_LIST_SCALERS, shdr.c_str());
		if (shdr == currentlyselected)
			selindex = resindex;
	}
	ListboxSetSelected(m_hWnd, IDC_LIST_SCALERS, selindex);
	if (selindex == -1)
		SetShaderDescription(L"");
	else
		SetShaderDescription(currentlyselected);
		
	
	testshader = nullptr;
}

void CD3D12SettingsPPage::SetShaderOptions(std::wstring file)
{
	std::wstring appdata = _wgetenv(L"APPDATA");
	appdata.append(L"\\MPCVideoRenderer\\Shaders\\");
	appdata.append(file);
	CShaderFileLoader* testshader = new CShaderFileLoader(file);
	ShaderDesc desc;
	testshader->Compile(desc, true);
	SetEditText(m_hWnd, IDC_STATIC_XBR_STR, L"");
	SetEditText(m_hWnd, IDC_STATIC_XBR_STR2, L"");
	SetEditText(m_hWnd, IDC_STATIC_XBR_STR3, L"");
	SetEditText(m_hWnd, IDC_STATIC_XBR_STR4, L"");
	SetEditText(m_hWnd, IDC_STATIC_XBR_STR5, L"");
	SetEditText(m_hWnd, IDC_STATIC_XBR_STR6, L"");
	GetDlgItem(IDC_SLIDER1).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SLIDER2).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SLIDER3).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SLIDER4).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SLIDER5).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SLIDER6).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT1).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT2).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT3).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT4).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT5).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT6).ShowWindow(SW_HIDE);
	int configidx = 0;
	for (ShaderConstantDesc sconst : desc.constants)
	{
		if (configidx == 0)
		{
			if (sconst.label.length()>0)
				SetEditText(m_hWnd, IDC_STATIC_XBR_STR, Utility::UTF8ToWideString(sconst.label));
			else
				SetEditText(m_hWnd, IDC_STATIC_XBR_STR, Utility::UTF8ToWideString(sconst.name));
			SetControlConfig(m_hWnd, IDC_EDIT1, IDC_SLIDER1, sconst);
			
		}
		else if (configidx == 1)
		{
			if (sconst.label.length() > 0)
				SetEditText(m_hWnd, IDC_STATIC_XBR_STR2, Utility::UTF8ToWideString(sconst.label));
			else
				SetEditText(m_hWnd, IDC_STATIC_XBR_STR2, Utility::UTF8ToWideString(sconst.name));
			SetControlConfig(m_hWnd, IDC_EDIT2, IDC_SLIDER2, sconst);
		}
		else if (configidx == 2)
		{
			if (sconst.label.length() > 0)
				SetEditText(m_hWnd, IDC_STATIC_XBR_STR3, Utility::UTF8ToWideString(sconst.label));
			else
				SetEditText(m_hWnd, IDC_STATIC_XBR_STR3, Utility::UTF8ToWideString(sconst.name));
			SetControlConfig(m_hWnd, IDC_EDIT3, IDC_SLIDER3, sconst);
		}
		else if (configidx == 3)
		{
			if (sconst.label.length() > 0)
				SetEditText(m_hWnd, IDC_STATIC_XBR_STR4, Utility::UTF8ToWideString(sconst.label));
			else
				SetEditText(m_hWnd, IDC_STATIC_XBR_STR4, Utility::UTF8ToWideString(sconst.name));
			SetControlConfig(m_hWnd, IDC_EDIT4, IDC_SLIDER4, sconst);
		}
		else if (configidx == 4)
		{
			if (sconst.label.length() > 0)
				SetEditText(m_hWnd, IDC_STATIC_XBR_STR5, Utility::UTF8ToWideString(sconst.label));
			else
				SetEditText(m_hWnd, IDC_STATIC_XBR_STR5, Utility::UTF8ToWideString(sconst.name));
			SetControlConfig(m_hWnd, IDC_EDIT5, IDC_SLIDER5, sconst);
		}
		else if (configidx == 5)
		{
			if (sconst.label.length() > 0)
				SetEditText(m_hWnd, IDC_STATIC_XBR_STR6, Utility::UTF8ToWideString(sconst.label));
			else
				SetEditText(m_hWnd, IDC_STATIC_XBR_STR6, Utility::UTF8ToWideString(sconst.name));
			SetControlConfig(m_hWnd, IDC_EDIT6, IDC_SLIDER6, sconst);
		}
		configidx += 1;
		

	}
	m_pCurrentShaderConstants = desc.constants;
	testshader = nullptr;
}

void CD3D12SettingsPPage::SetShaderDescription(std::wstring file)
{
	if (file.size() == 0)
	{
		SetEditText(m_hWnd, IDC_EDIT_DESC, L"");
		return;
	}
		std::wstring appdata = _wgetenv(L"APPDATA");
	appdata.append(L"\\MPCVideoRenderer\\Shaders\\");
	appdata.append(file);
	CShaderFileLoader* testshader = new CShaderFileLoader(L"");
	
	SetEditText(m_hWnd, IDC_EDIT_DESC, Utility::UTF8ToWideString(testshader->GetScalerDescription(appdata)));
	testshader = nullptr;
}

INT_PTR CD3D12SettingsPPage::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_HSCROLL)
	{
		int cValue = HIWORD(wParam);
		const int nID = GetSliderID((HWND)lParam);
		//cValue
		if (cValue == 0)
		{
			switch (nID)
			{
			case IDC_SLIDER1:
				cValue = GetPos(m_hWnd, IDC_SLIDER1);
				break;
			case IDC_SLIDER2:
				cValue = GetPos(m_hWnd, IDC_SLIDER2);
				break;
			case IDC_SLIDER3:
				cValue = GetPos(m_hWnd, IDC_SLIDER3);
				break;
			case IDC_SLIDER4:
				cValue = GetPos(m_hWnd, IDC_SLIDER4);
				break;
			case IDC_SLIDER5:
				cValue = GetPos(m_hWnd, IDC_SLIDER5);
				break;
			case IDC_SLIDER6:
				cValue = GetPos(m_hWnd, IDC_SLIDER6);
				break;
			}
		}
		if (nID == IDC_SLIDER1)
		{
			UpdateScroll(0, IDC_EDIT1,cValue);
			return 0;
			
		}
		else if (nID == IDC_SLIDER2)
		{
			UpdateScroll(1, IDC_EDIT2, cValue);
			return 0;
		}
		else if (nID == IDC_SLIDER3)
		{
			UpdateScroll(2, IDC_EDIT3, cValue);
			return 0;
		}
		else if (nID == IDC_SLIDER4)
		{
			UpdateScroll(3, IDC_EDIT4, cValue);
			return 0;
		}
		else if (nID == IDC_SLIDER5)
		{
			UpdateScroll(4, IDC_EDIT5, cValue);
			return 0;
		}
		else if (nID == IDC_SLIDER6)
		{
			UpdateScroll(5, IDC_EDIT6, cValue);
			return 0;
		}
	}

	if (uMsg == WM_COMMAND)
	{

		const int nID = LOWORD(wParam);
		//Scaler selection changed
		if (IDC_RADIO1 <= nID && nID <= IDC_RADIO5 && HIWORD(wParam) == BN_CLICKED)
		{
			ListboxClear(m_hWnd, IDC_LIST_SCALERS);
			if (nID == IDC_RADIO1 && m_sCurrentUpScaler.length()>0)
			{
				FillScalers(ScalerType::Upscaler);
			}
			else if (nID == IDC_RADIO2 && m_sCurrentDownScaler.length() > 0)
			{
				FillScalers(ScalerType::Downscaler);
			}
			else if (nID == IDC_RADIO3 && m_sCurrentChromaUpscaler.length() > 0)
			{
				FillScalers(ScalerType::Chromaupscaler);
			}
			else if (nID == IDC_RADIO4 && m_sCurrentImageDoubler.length() > 0)
			{
				FillScalers(ScalerType::ImageDouble);
			}
			else if (nID == IDC_RADIO5 && m_sCurrentPostScaler.length() > 0)
			{
				FillScalers(ScalerType::PostShader);
			}
			return 1l;
		}
		//listbox of current scaler changed
		//m_pCurrentScalerType
		if (nID == IDC_LIST_SCALERS)
		{
			std::wstring curscaler = ListboxGetCurrentText(m_hWnd,IDC_LIST_SCALERS);
			if (m_pCurrentScalerType == ScalerType::Upscaler && curscaler.length() > 0 && m_sCurrentUpScaler != curscaler)
			{
				m_sCurrentUpScaler = curscaler;
				D3D12Engine::g_D3D12Options->SetCurrentUpscaler(curscaler);
				SetShaderDescription(curscaler);
				SetShaderOptions(curscaler);
			}
			else if (m_pCurrentScalerType == ScalerType::Downscaler && curscaler.length() > 0)
			{
				D3D12Engine::g_D3D12Options->SetCurrentDownscaler(curscaler);
			}
			else if (m_pCurrentScalerType == ScalerType::Chromaupscaler && curscaler.length() > 0)
			{
				D3D12Engine::g_D3D12Options->SetCurrentChromaUpscaler(curscaler);
			}
			else if (m_pCurrentScalerType == ScalerType::ImageDouble && curscaler.length() > 0)
			{
				D3D12Engine::g_D3D12Options->SetCurrentImageDoubler(curscaler);
			}
			else if (m_pCurrentScalerType == ScalerType::PostShader && curscaler.length() > 0)
			{
				D3D12Engine::g_D3D12Options->SetCurrentPostShader(curscaler);
			}
			return 1l;
		}

	}
#if 0
		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			eD3D12Upscalers currentupscaler = (eD3D12Upscalers)D3D12Engine::g_D3D12Options->GetCurrentUpscaler();
			if (nID == IDC_COMBO_OPTIONS && currentupscaler == eD3D12Upscalers::lanczos)
			{
				lValue = SendDlgItemMessageW(IDC_COMBO_OPTIONS, CB_GETCURSEL, 0, 0);
				lValue += 2;//lanczos 2 and 3
				CScalerOption* opt = D3D12Engine::g_D3D12Options->GetScaler("lanczos");
				if (opt->GetInt("lanczostype") != lValue)
				{
					SetDirty();
					opt->SetInt("lanczostype", lValue);
					D3D12Engine::g_D3D12Options->SetScaler("lanczos",opt);
				}
				return (LRESULT)1;
			}
			else if (nID == IDC_COMBO_OPTIONS && currentupscaler == eD3D12Upscalers::spline)
			{
				lValue = SendDlgItemMessageW(IDC_COMBO_OPTIONS, CB_GETCURSEL, 0, 0);
				CScalerOption* opt = D3D12Engine::g_D3D12Options->GetScaler("spline");
				if (opt->GetInt("splinetype") != lValue)
				{
					SetDirty();
					opt->SetInt("splinetype", lValue);
					D3D12Engine::g_D3D12Options->SetScaler("spline", opt);
				}
				return (LRESULT)1;
			}
		}
		if (nID == IDC_CHECK13)
		{
			m_SetsPP.D3D12Settings.bUseD3D12 = IsDlgButtonChecked(IDC_CHECK13) == BST_CHECKED;
			EnableControls();
			SetDirty();
			return (LRESULT)1;
		}

		if (nID == IDC_CHECK17)
		{
			m_SetsPP.D3D12Settings.bForceD3D12 = IsDlgButtonChecked(IDC_CHECK17) == BST_CHECKED;
			EnableControls();
			SetDirty();
			return (LRESULT)1;
		}

		if (IDC_RADIO_DOWNSCALING1 <= nID && nID <= IDC_RADIO_DOWNSCALING6 && HIWORD(wParam) == BN_CLICKED)
		{
			//0 based and we got 6 value so 5 - (IDC_RADIO_DOWNSCALING6 - currentvalue)
			int currentbutton = 5 - (IDC_RADIO_DOWNSCALING6 - nID);
			lValue = GetRadioValue(m_hWnd, nID);
			if (currentbutton != (m_iCurrentDownScaler))
			{
				SetDirty();
				m_iCurrentDownScaler = currentbutton;
			}
		}
		else if (IDC_RADIO_UPSCALING1 <= nID && nID <= IDC_RADIO_UPSCALING9 && HIWORD(wParam) == BN_CLICKED)
		{
			int currentbutton = 8 - (IDC_RADIO_UPSCALING9 - nID);
			m_iCurrentUpScaler = currentbutton;
			lValue = GetRadioValue(m_hWnd, nID);

			if (currentbutton != (D3D12Engine::g_D3D12Options->GetCurrentUpscaler()))
			{
				D3D12Engine::g_D3D12Options->SetCurrentUpscaler(currentbutton);
				UpdateCurrentScaler();
				SetDirty();
				
			}

		}
		else if (IDC_RADIO_CHROMAUP1 <= nID && nID <= IDC_RADIO_CHROMAUP9 && HIWORD(wParam) == BN_CLICKED)
		{
			int currentbutton = 9 - (IDC_RADIO_CHROMAUP9 - nID);//IDC_RADIO_CHROMAUP1 + LOWORD(wParam);
			lValue = GetRadioValue(m_hWnd, nID);
			
			if (currentbutton != (m_sCurrentChromaUpscaler))
			{
				SetDirty();
				m_iCurrentChromaUpscaler = currentbutton;
			}
		}
	}
#endif

	return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

HRESULT CD3D12SettingsPPage::OnApplyChanges()
{
	D3D12Engine::g_D3D12Options->SaveCurrentSettings();
	m_pVideoRenderer->SetSettings(m_SetsPP);
	m_pVideoRenderer->SaveSettings();
	
	return S_OK;
}
