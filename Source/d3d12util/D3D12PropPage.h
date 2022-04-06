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
#include <map>

// CD3D12SettingsPPage

class __declspec(uuid("465D2B19-DE41-4425-8666-FB7FF0DAF122"))
	CD3D12SettingsPPage : public CBasePropertyPage, public CWindow
{
	CComQIPtr<IVideoRenderer> m_pVideoRenderer;
	Settings_t m_SetsPP;

public:
	CD3D12SettingsPPage(LPUNKNOWN lpunk, HRESULT* phr);
	~CD3D12SettingsPPage();

	void EnableControls();
private:
	
	HRESULT OnConnect(IUnknown* pUnknown) override;
	HRESULT OnDisconnect() override;
	HRESULT OnActivate() override;
	INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	HRESULT OnApplyChanges();

	void SetDirty()
	{
		m_bDirty = TRUE;
		if (m_pPageSite)
		{
			m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
		}
	}

};
