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

#include "stdafx.h"
#include "resource.h"
#include "Helper.h"
#include "DisplayConfig.h"
#include "D3D12PropPage.h"
#include "PropPage.h"
#include "Dx12Engine.h"
#include <iostream>
#include <filesystem>
#include "FileUtility.h"
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

inline int ListboxGetItemCount(HWND hWnd, int nIDListBox)
{
	return SendDlgItemMessageW(hWnd, nIDListBox, LB_GETCOUNT, 0, 0);
	
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
	if (!m_pVideoRenderer)
		return E_NOINTERFACE;
	

	return S_OK;
}

HRESULT CD3D12SettingsPPage::OnDisconnect()
{
	if (m_pVideoRenderer == nullptr)
		return E_UNEXPECTED;
	

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

	if (!D3D12Engine::g_Options)
		D3D12Engine::g_Options = new CD3D12Options();
	m_sCurrentUpScaler = D3D12Engine::g_Options->GetCurrentUpscaler();
	m_sCurrentChromaUpscaler = D3D12Engine::g_Options->GetCurrentChromaUpscaler();
	m_sCurrentDownScaler = D3D12Engine::g_Options->GetCurrentDownscaler();
	m_sCurrentImageDoubler = D3D12Engine::g_Options->GetCurrentImageDoubler();
	m_sCurrentPostScaler = D3D12Engine::g_Options->GetCurrentPostscaler();

	//todo postscaler m_sCurrentDownScaler = D3D12Engine::g_Options->get();
	SetRadioValue(m_hWnd, IDC_RADIO1, BM_SETCHECK, BST_CHECKED, 0);
	FillScalers(ScalerType::Upscaler);
	SetShaderOptions(m_sCurrentUpScaler);
	CheckDlgButton(IDC_CHECK13, m_SetsPP.D3D12Settings.bUseD3D12 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK17, m_SetsPP.D3D12Settings.bForceD3D12 ? BST_CHECKED : BST_UNCHECKED);
	//UpdateCurrentScaler();
	GetDlgItem(IDC_LIST_POSTSCALERS).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BUTTON_ADD).ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BUTTON_REMOVE).ShowWindow(SW_HIDE);
	return S_OK;
}

void CD3D12SettingsPPage::UpdateCurrentPostScaler()
{

}

void CD3D12SettingsPPage::UpdateCurrentScaler(std::vector<ShaderConstantDesc> shaderconst)
{
	CScalerOption* opt = nullptr;
	std::wstring type = Utility::UTF8ToWideString(s_scalertype[Upscaler]);
	switch (m_pCurrentScalerType)
	{
	case Upscaler:
		opt = D3D12Engine::g_Options->GetScaler(m_sCurrentUpScaler, type);
		break;
	case Downscaler:
		opt = D3D12Engine::g_Options->GetScaler(m_sCurrentDownScaler, type);
		break;
	case Chromaupscaler:
		opt = D3D12Engine::g_Options->GetScaler(m_sCurrentChromaUpscaler, type);
		break;
	case ImageDouble:
		opt = D3D12Engine::g_Options->GetScaler(m_sCurrentImageDoubler, type);
		break;
	case PostShader:
	default:
		return;
		break;
	}
	if (!opt)
		return;
	for (ShaderConstantDesc xx : shaderconst)
	{
		if (xx.type == ShaderConstantType::Int)
			opt->SetInt(xx.label, std::get<int>(xx.currentValue));
		else if (xx.type == ShaderConstantType::Float)
			opt->SetInt(xx.label, std::get<float>(xx.currentValue));
	}

	D3D12Engine::g_Options->SetScaler(opt->m_pScalerName, s_scalertype[Upscaler],opt);
	

	opt = nullptr;

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
	//update loaded shaders
	if (D3D12Engine::m_pCurrentUpScaler)
		D3D12Engine::m_pCurrentUpScaler->SetShaderConstants(m_pCurrentShaderConstants);
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
		SetPos(hwnd, slideridx, (std::get<float>(sconst.currentValue) * 100));
		SetEditText(hwnd, editidx, std::get<float>(sconst.currentValue));
	}
	else
	{
		if (sconst.maxValue.index() != 0)
			SetRangeMinMax(hwnd, slideridx, (std::get<int>(sconst.minValue)), (std::get<int>(sconst.maxValue)));
		else
			SetRangeMinMax(hwnd, slideridx, (std::get<int>(sconst.minValue)), 100);
		SetPos(hwnd, slideridx, (std::get<int>(sconst.currentValue)));
		SetEditText(hwnd, editidx, std::get<int>(sconst.currentValue));
	}

}
void CD3D12SettingsPPage::FillScalers(ScalerType scalertype)
{
	std::wstring appdata = Utility::GetDirAppData();
	appdata.append(L"\\MPCVideoRenderer\\Shaders\\");
	std::filesystem::directory_iterator end_itr; // default construction yields past-the-end

	std::vector<std::wstring> files = Utility::GetAllHlslInFolder(appdata);
	CShaderFileLoader* testshader = new CShaderFileLoader(L"");
	std::vector<std::wstring> shaders;
	std::string scl;
	m_pCurrentScalerType = scalertype;
	GetDlgItem(IDC_LIST_POSTSCALERS).ShowWindow((scalertype != ScalerType::PostShader) ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_BUTTON_ADD).ShowWindow((scalertype != ScalerType::PostShader) ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_BUTTON_REMOVE).ShowWindow((scalertype != ScalerType::PostShader) ? SW_HIDE : SW_SHOW);
	//if (scalertype == ScalerType::PostShader)
	for (std::wstring fle : files)
	{
		scl = testshader->GetScalerType(appdata + fle);

		if (scl.find_first_of("UPSCALER") != std::string::npos && scalertype == ScalerType::Upscaler)
		{
			shaders.push_back(fle);
		}
		else if (scl.find_first_of("DOWNSCALER") != std::string::npos && scalertype == ScalerType::Downscaler)
		{
			shaders.push_back(fle);
		}
		else if (scl.find_first_of("CHROMASCALER") != std::string::npos && scalertype == ScalerType::Chromaupscaler)
		{
			shaders.push_back(fle);
		}
		else if (scl.find("IMAGEDOUBLER") != std::string::npos && scalertype == ScalerType::ImageDouble)
		{
			shaders.push_back(fle);
		}
		/*else if (scl.find("POST") != std::string::npos && scalertype == ScalerType::PostShader)*/
		//{
		shaders.push_back(fle);
		//}
	}

	LRESULT resindex = -1;
	LRESULT selindex = -1;
	std::wstring currentlyselected;
	bool postshader = false;
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
		postshader = true;
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
	
	if (!postshader)
	{
		ListboxSetSelected(m_hWnd, IDC_LIST_SCALERS, selindex);
		if (selindex == -1)
			SetShaderDescription(L"");
		else
			SetShaderDescription(currentlyselected);
	}
	else
	{
		//select the first one
		ListboxClear(m_hWnd, IDC_LIST_POSTSCALERS);
		ListboxSetSelected(m_hWnd, IDC_LIST_SCALERS, 0);
		if (m_sCurrentPostScaler.size() > 0)
		{
			for (std::wstring xx : m_sCurrentPostScaler)
				ListboxAddString(m_hWnd, IDC_LIST_POSTSCALERS, xx.c_str());

			SetShaderDescription(m_sCurrentPostScaler.at(0));
			SetShaderOptions(m_sCurrentPostScaler.at(0));
		}
		ListboxSetSelected(m_hWnd, IDC_LIST_POSTSCALERS, 0);
	}
	
	
	
	testshader = nullptr;
}

void CD3D12SettingsPPage::SetShaderOptions(std::wstring file)
{
	std::wstring appdata = Utility::GetDirAppData();
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
	//add scaler to option if it does not exist
	CScalerOption* opt;
	bool scalerHasConfig = true;
	std::string sclnameA = Utility::WideStringToUTF8(file);
	
	opt = D3D12Engine::g_Options->GetScaler(sclnameA.c_str(),s_scalertype[m_pCurrentScalerType]);

	if (!opt)
	{
		opt = D3D12Engine::g_Options->CreateScaler(sclnameA, s_scalertype[m_pCurrentScalerType]);
		scalerHasConfig = false;
	}
	
	std::string type = opt->GetString("type");
	//need this when we add post shaders
	if (opt->HasString("new") && opt->GetString("new") == "YES")
		scalerHasConfig = false;


	for (ShaderConstantDesc sconst : desc.constants)
	{
		if (!scalerHasConfig)
		{
			if (sconst.type == ShaderConstantType::Float)
			{
				opt->AddFloat(sconst.name.c_str(), fmt::to_string(std::get<float>(sconst.defaultValue)));
			}
			else if (sconst.type == ShaderConstantType::Int)
				opt->AddInt(sconst.name.c_str(), fmt::to_string(std::get<int>(sconst.defaultValue)));
			sconst.currentValue = sconst.defaultValue;
		}
		else
		{
			if (sconst.type == ShaderConstantType::Float)
				sconst.currentValue = opt->GetFloat(sconst.name.c_str());
			else if (sconst.type == ShaderConstantType::Int)
				sconst.currentValue = opt->GetInt(sconst.name.c_str());
		}
		
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

	if (opt->HasOptions())
		D3D12Engine::g_Options->SetScaler(opt->m_pScalerName, s_scalertype[m_pCurrentScalerType], opt);
	opt = nullptr;
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
std::wstring appdata = Utility::GetDirAppData();
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
		SetDirty();
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

	if (uMsg == WM_LBUTTONDOWN)
	{
		//TODO drag and drop 
		if (LOWORD(wParam) == IDC_LIST_POSTSCALERS)
		{
			m_pDragPostScaler = CPoint((int)LOWORD(lParam), (int)(short)HIWORD(lParam));
			
		}
	}// == IDC_LIST_POSTSCALERS)
	if (uMsg == WM_COMMAND)
	{
		const int nID = LOWORD(wParam);
		//Scaler selection changed
		if (IDC_RADIO1 <= nID && nID <= IDC_RADIO5 && HIWORD(wParam) == BN_CLICKED)
		{
			ListboxClear(m_hWnd, IDC_LIST_SCALERS);
			if (nID == IDC_RADIO1)
			{
				FillScalers(ScalerType::Upscaler);
			}
			else if (nID == IDC_RADIO2)
			{
				FillScalers(ScalerType::Downscaler);
			}
			else if (nID == IDC_RADIO3)
			{
				FillScalers(ScalerType::Chromaupscaler);
			}
			else if (nID == IDC_RADIO4)
			{
				FillScalers(ScalerType::ImageDouble);
			}
			else if (nID == IDC_RADIO5)
			{
				FillScalers(ScalerType::PostShader);
			}
			return 1l;
		}
		if (HIWORD(wParam) == BN_CLICKED && nID == IDC_BUTTON_ADD)
		{
			//shouldn't happen
			if (m_pCurrentScalerType != ScalerType::PostShader)
				assert(0);
			std::wstring curscaler = ListboxGetCurrentText(m_hWnd, IDC_LIST_SCALERS);
			ListboxAddString(m_hWnd, IDC_LIST_POSTSCALERS, curscaler.c_str());
			SetDirty();
			m_sCurrentPostScaler.push_back(curscaler);
			D3D12Engine::g_Options->AddPostScaler(curscaler);
			return 1l;
		}
		if (HIWORD(wParam) == BN_CLICKED && nID == IDC_BUTTON_REMOVE)
		{
			//shouldn't happen
			if (m_pCurrentScalerType != ScalerType::PostShader)
				assert(0);
			//too lazy right now will just remove and recreate
			std::wstring curscaler = ListboxGetCurrentText(m_hWnd, IDC_LIST_POSTSCALERS);
			std::vector<std::wstring>::iterator position = std::find(m_sCurrentPostScaler.begin(), m_sCurrentPostScaler.end(), curscaler);
			if (position != m_sCurrentPostScaler.end()) // == myVector.end() means the element was not found
				m_sCurrentPostScaler.erase(position);
			
			
			ListboxClear(m_hWnd, IDC_LIST_POSTSCALERS);
			for (std::wstring xx : m_sCurrentPostScaler)
				ListboxAddString(m_hWnd, IDC_LIST_POSTSCALERS, xx.c_str());
			SetDirty();

			D3D12Engine::g_Options->RemovePostScaler(curscaler);
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
				D3D12Engine::g_Options->SetCurrentUpscaler(curscaler);
			}
			else if (m_pCurrentScalerType == ScalerType::Downscaler && curscaler.length() > 0 && m_sCurrentDownScaler != curscaler)
			{
				D3D12Engine::g_Options->SetCurrentDownscaler(curscaler);
			}
			else if (m_pCurrentScalerType == ScalerType::Chromaupscaler && curscaler.length() > 0 && m_sCurrentChromaUpscaler != curscaler)
			{
				D3D12Engine::g_Options->SetCurrentChromaUpscaler(curscaler);
			}
			else if (m_pCurrentScalerType == ScalerType::ImageDouble && curscaler.length() > 0 && m_sCurrentImageDoubler != curscaler)
			{
				D3D12Engine::g_Options->SetCurrentImageDoubler(curscaler);
			}

			if (curscaler.length() > 0)
			{
				SetShaderDescription(curscaler);
				SetShaderOptions(curscaler);
			}
			return 1l;
		}
		if (nID == IDC_LIST_POSTSCALERS)
		{
			std::wstring curscaler = ListboxGetCurrentText(m_hWnd, IDC_LIST_POSTSCALERS);
			if (curscaler.length() > 0)
			{
				SetShaderDescription(curscaler);
				SetShaderOptions(curscaler);
			}
			return 1l;
		}
		if (nID == IDC_CHECK13)
		{
			m_SetsPP.D3D12Settings.bUseD3D12 = IsDlgButtonChecked(IDC_CHECK13) == BST_CHECKED;
			SetDirty();
			return (LRESULT)1;
		}

		if (nID == IDC_CHECK17)
		{
			m_SetsPP.D3D12Settings.bForceD3D12 = IsDlgButtonChecked(IDC_CHECK17) == BST_CHECKED;
			SetDirty();
			return (LRESULT)1;
		}
	}

	return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

HRESULT CD3D12SettingsPPage::OnApplyChanges()
{
	//todo fix beter than this removing the postscaler
	if (ListboxGetItemCount(m_hWnd, IDC_LIST_POSTSCALERS) == 0)
	{
		D3D12Engine::g_Options->RemovePostScaler(L"*");
		m_sCurrentPostScaler.clear();

	}
	D3D12Engine::g_Options->SaveCurrentSettings();
	
	m_pVideoRenderer->SetSettings(m_SetsPP);
	m_pVideoRenderer->SaveSettings();
	
	return S_OK;
}
