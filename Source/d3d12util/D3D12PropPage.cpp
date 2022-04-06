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
	//build the tree
	
	return S_OK;
}



INT_PTR CD3D12SettingsPPage::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lValue;
	BOOL bValue;
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
		else if (IDC_RADIO_UPSCALING1 <= nID && nID <= IDC_RADIO_UPSCALING7 && HIWORD(wParam) == BN_CLICKED)
		{
			int currentbutton = 6 - (IDC_RADIO_UPSCALING7 - nID);
			lValue = GetRadioValue(m_hWnd, nID);

			if (currentbutton != (m_SetsPP.D3D12Settings.imageUpscaling))
			{
				SetDirty();
				m_SetsPP.D3D12Settings.imageUpscaling = currentbutton;
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
