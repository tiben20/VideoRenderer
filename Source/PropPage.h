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

#include "IVideoRenderer.h"


void SetCursor(HWND hWnd, LPCWSTR lpCursorName);
void SetCursor(HWND hWnd, UINT nID, LPCWSTR lpCursorName);
inline void ComboBox_AddStringData(HWND hWnd, int nIDComboBox, LPCWSTR str, LONG_PTR data);

inline LONG_PTR ComboBox_GetCurItemData(HWND hWnd, int nIDComboBox);

void ComboBox_SelectByItemData(HWND hWnd, int nIDComboBox, LONG_PTR data);

// CVRInfoPPage

class __declspec(uuid("D697132B-FCA4-4401-8869-D3B39D0750DB"))
	CVRInfoPPage : public CBasePropertyPage, public CWindow
{
	HFONT m_hMonoFont = nullptr;
	CComQIPtr<IVideoRenderer> m_pVideoRenderer;

public:
	CVRInfoPPage(LPUNKNOWN lpunk, HRESULT* phr);
	~CVRInfoPPage();

private:
	HRESULT OnConnect(IUnknown* pUnknown) override;
	HRESULT OnDisconnect() override;
	HRESULT OnActivate() override;
	INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};
