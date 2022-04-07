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

template<typename T> inline void SetEditText(HWND hWnd, int nID, T value)
{
	
	std::wstring str = fmt::format(L"{}", value);
	SendDlgItemMessageW(hWnd, nID, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(str.c_str()));
}

inline int GetSliderID(HWND hWnd)
{
	return GetDlgCtrlID(hWnd);
}
// CD3D12SettingsPPage

CD3D12SettingsPPage::CD3D12SettingsPPage(LPUNKNOWN lpunk, HRESULT* phr) :
	CBasePropertyPage(L"D3D12Prop", lpunk, IDD_D3D12PROPPAGE, IDS_D3D12PROP_TITLE)
{
	DLog(L"CD3D12SettingsPPage()");
	m_pTreeRoot = 0;
	m_pTreeGeneral = 0;
	m_pTreeUpscale = 0;
	m_pTreeDownscale = 0;
	m_pTreeChroma = 0;
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

	m_pVideoRenderer->GetSettings(m_SetsPP);

	int i;
	for (i = 0; i < 9; i++)
		SetRadioValue(m_hWnd, IDC_RADIO_CHROMAUP1 + i, BM_SETCHECK, (m_SetsPP.D3D12Settings.chromaUpsampling == (i)), 0);
	//for (i = 0; i < 5; i++)
		//SetRadioValue(m_hWnd, IDC_RADIO_DOUBLING1 + i, BM_SETCHECK, (m_SetsPP.D3D12Settings.imageUpscalingDoubling == (i)), 0);
	for (i = 0; i < 7; i++)
		SetRadioValue(m_hWnd, IDC_RADIO_UPSCALING1 + i, BM_SETCHECK, (m_SetsPP.D3D12Settings.imageUpscaling == (i)), 0);
	for (i = 0; i < 6; i++)
		SetRadioValue(m_hWnd, IDC_RADIO_DOWNSCALING1 + i, BM_SETCHECK, (m_SetsPP.D3D12Settings.imageDownscaling == (i)), 0);

	CheckDlgButton(IDC_CHECK13, m_SetsPP.D3D12Settings.bUseD3D12 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK17, m_SetsPP.D3D12Settings.bForceD3D12 ? BST_CHECKED : BST_UNCHECKED);
	
	SetRangeMinMax(m_hWnd, IDC_SLIDER1, 0,5);//str default 1
	SetRangeMinMax(m_hWnd, IDC_SLIDER2, 0, 15);//sharp 0 to 1.5  default 1
	SetRangeMinMax(m_hWnd, IDC_SLIDER3, 0, 3);//factor 2, 4 , 8 , 16 default 2
	SetPos(m_hWnd, IDC_SLIDER1, (m_SetsPP.D3D12Settings.xbrConfig.iStrength));
	SetEditText(m_hWnd, IDC_EDIT1, m_SetsPP.D3D12Settings.xbrConfig.iStrength);
	SetPos(m_hWnd, IDC_SLIDER2, (m_SetsPP.D3D12Settings.xbrConfig.fSharp * 10));
	SetEditText(m_hWnd, IDC_EDIT2, m_SetsPP.D3D12Settings.xbrConfig.fSharp);
	SetPos(m_hWnd, IDC_SLIDER3, (m_SetsPP.D3D12Settings.xbrConfig.iFactor));
	SetEditText(m_hWnd, IDC_EDIT3, m_SetsPP.D3D12Settings.xbrConfig.iStrength);
	//SetEditText(m_hWnd, IDC_STATIC_XBR_STR, L"XBR");
	
	UpdateCurrentScaler();
	return S_OK;
}

void CD3D12SettingsPPage::UpdateCurrentScaler()
{
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
	GetDlgItem(IDC_COMBO_OPTIONS).ShowWindow(SW_HIDE);
	
	GetDlgItem(IDC_EDIT1).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT2).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT3).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT4).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT5).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_EDIT6).ShowWindow(SW_HIDE);
	ResetContent(m_hWnd, IDC_COMBO_OPTIONS);
	//bilinear
	if (m_SetsPP.D3D12Settings.imageUpscaling == 0)
	{
		
	}
	//DXVA2
	if (m_SetsPP.D3D12Settings.imageUpscaling == 1)
	{
		
	}
	//Bicubuic
	if (m_SetsPP.D3D12Settings.imageUpscaling == 2)
	{
		//no options
		GetDlgItem(IDC_COMBO_OPTIONS).ShowWindow(SW_SHOW);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Mitchel-Netravali", 0);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Bicubic 50 / Catmull-Rom", 1);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Bicubic 60", 2);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Bicubic 75", 3);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Bicubic 100", 4);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Bicubic 125", 5);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Bicubic 150", 6);
		//
		//ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Bicubic 60", 2);
		//ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"Bicubic 60", 2);
	}
	//lanczso
	if (m_SetsPP.D3D12Settings.imageUpscaling == 3)
	{
		GetDlgItem(IDC_COMBO_OPTIONS).ShowWindow(SW_SHOW);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"3 Taps", 0);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"4 Taps", 1);
	}
	//Spline
	if (m_SetsPP.D3D12Settings.imageUpscaling == 4)
	{
		GetDlgItem(IDC_COMBO_OPTIONS).ShowWindow(SW_SHOW);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"3 Taps", 0);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"4 Taps", 1);
	}
	//Jinc
	if (m_SetsPP.D3D12Settings.imageUpscaling == 5)
	{
		
	}
	//super xbr
	if (m_SetsPP.D3D12Settings.imageUpscaling == 6)
	{
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
	}
	//Super res
	if (m_SetsPP.D3D12Settings.imageUpscaling == 7)
	{
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
		GetDlgItem(IDC_COMBO_OPTIONS).ShowWindow(SW_SHOW);
		ComboBox_AddStringData(m_hWnd, IDC_COMBO_OPTIONS, L"spline resize", 0);
		//dither tools
		//planar upscale
	}
	//Super res + xbr
	if (m_SetsPP.D3D12Settings.imageUpscaling == 8)
	{
		SetRangeMinMax(m_hWnd, IDC_SLIDER1, 0, 5);//str default 1
		SetRangeMinMax(m_hWnd, IDC_SLIDER2, 0, 15);//sharp 0 to 1.5  default 1
		SetRangeMinMax(m_hWnd, IDC_SLIDER3, 0, 3);//factor 2, 4 , 8 , 16 default 2
		SetRangeMinMax(m_hWnd, IDC_SLIDER4, 1, 5); //passes
		SetRangeMinMax(m_hWnd, IDC_SLIDER5, 0, 10);//float 0 to 1
		SetRangeMinMax(m_hWnd, IDC_SLIDER6, 0, 10);//float 0 to 1
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR, L"Xbr Strength");
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR2, L"Sharp");
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR3, L"Factor");
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR4, L"Xbr Strength");
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR5, L"Sharp");
		SetEditText(m_hWnd, IDC_STATIC_XBR_STR6, L"Factor");
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
	}
	
}
INT_PTR CD3D12SettingsPPage::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lValue;
	BOOL bValue;
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
			}
		}
		if (nID == IDC_SLIDER1 && cValue != m_SetsPP.D3D12Settings.xbrConfig.iStrength)
		{
			//cValue != m_SetsPP.D3D12Settings.xbrConfig.iStrength)
				m_SetsPP.D3D12Settings.xbrConfig.iStrength = cValue;
				SetDirty();
				SetEditText(m_hWnd, IDC_EDIT1, m_SetsPP.D3D12Settings.xbrConfig.iStrength);
				return 0;
		}
		else if (nID == IDC_SLIDER2 && cValue != m_SetsPP.D3D12Settings.xbrConfig.fSharp)
		{
			m_SetsPP.D3D12Settings.xbrConfig.fSharp = (float)cValue/10;
			SetDirty();
			SetEditText(m_hWnd, IDC_EDIT2, m_SetsPP.D3D12Settings.xbrConfig.fSharp);
			return 0;
		}
		else if (nID == IDC_SLIDER3 && cValue != m_SetsPP.D3D12Settings.xbrConfig.iFactor)
		{
			m_SetsPP.D3D12Settings.xbrConfig.iFactor = cValue;
			SetDirty();
			int fact;
			switch (m_SetsPP.D3D12Settings.xbrConfig.iFactor)
			{
			case 0:
				fact = 2;
				break;
			case 1:
				fact = 4;
				break;
			case 2:
				fact = 8; 
				break;
			case 3:
				fact = 16;
				break;
			default:
				fact = 2;
				break;
			}
			SetEditText(m_hWnd, IDC_EDIT3, fact);
			return 0;
			
		}
	}
	if (uMsg == WM_COMMAND) 
	{
		const int nID = LOWORD(wParam);

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
			if (currentbutton != (m_SetsPP.D3D12Settings.imageDownscaling))
			{
				SetDirty();
				m_SetsPP.D3D12Settings.imageDownscaling = currentbutton;
			}
		}
		else if (IDC_RADIO_UPSCALING1 <= nID && nID <= IDC_RADIO_UPSCALING9 && HIWORD(wParam) == BN_CLICKED)
		{
			int currentbutton = 8 - (IDC_RADIO_UPSCALING9 - nID);
			lValue = GetRadioValue(m_hWnd, nID);

			if (currentbutton != (m_SetsPP.D3D12Settings.imageUpscaling))
			{
				m_SetsPP.D3D12Settings.imageUpscaling = currentbutton;
				UpdateCurrentScaler();
				SetDirty();
				
			}

		}
		/*else if (IDC_RADIO_DOUBLING1 <= nID && nID <= IDC_RADIO_DOUBLING5 && HIWORD(wParam) == BN_CLICKED)
		{
			int currentbutton = 4 - (IDC_RADIO_DOUBLING5 - nID);
			lValue = GetRadioValue(m_hWnd, nID);

			if (currentbutton != (m_SetsPP.D3D12Settings.imageUpscalingDoubling))
			{
				
				SetDirty();
				m_SetsPP.D3D12Settings.imageUpscalingDoubling = currentbutton;
			}
		}*/
		else if (IDC_RADIO_CHROMAUP1 <= nID && nID <= IDC_RADIO_CHROMAUP9 && HIWORD(wParam) == BN_CLICKED)
		{
			int currentbutton = 9 - (IDC_RADIO_CHROMAUP9 - nID);//IDC_RADIO_CHROMAUP1 + LOWORD(wParam);
			lValue = GetRadioValue(m_hWnd, nID);
			
			if (currentbutton != (m_SetsPP.D3D12Settings.chromaUpsampling))
			{
				SetDirty();
				m_SetsPP.D3D12Settings.chromaUpsampling = currentbutton;
			}
		}
	}


	return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

HRESULT CD3D12SettingsPPage::OnApplyChanges()
{
	m_pVideoRenderer->SetSettings(m_SetsPP);
	m_pVideoRenderer->SaveSettings();
	
	return S_OK;
}



void CD3D12SettingsPPage::EnableControls()
{
	if (!IsWindows8OrGreater()) { // Windows 7
		const BOOL bEnable = !m_SetsPP.D3D12Settings.bUseD3D12;
		int i;
		for (i = 0; i < 9; i++)
			GetDlgItem(IDC_RADIO_CHROMAUP1+i).EnableScrollBar(bEnable);
		for (i = 0; i < 5; i++)
			GetDlgItem(IDC_RADIO_DOUBLING1 + i).EnableScrollBar(bEnable);
		for (i = 0; i < 6; i++)
			GetDlgItem(IDC_RADIO_UPSCALING1 + i).EnableScrollBar(bEnable);
		for (i = 0; i < 6; i++)
			GetDlgItem(IDC_RADIO_DOWNSCALING1 + i).EnableScrollBar(bEnable);
	}
	
}
