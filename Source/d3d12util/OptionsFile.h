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


#include <vector>
#include <string>
#include <rpc.h>
#include <ppl.h>
#include "ppltasks.h"
#include <map>

class CScalerOption
{
public:
	CScalerOption(const char* name) { m_pScalerName = name; }
	~CScalerOption() {};
	void AddInt(const char* name, std::string value);
	void AddFloat(const char* name, std::string value);
	void AddString(const char* name, std::string value) { m_pStringOptions.insert({ name,value }); }
	int GetInt(std::string name) { return m_pIntOptions[name.c_str()]; }
	float GetFloat(std::string name) { return m_pFloatOptions[name.c_str()]; }
	std::string GetString(std::string name) { return m_pStringOptions[name.c_str()]; }
	std::string m_pScalerName;
	void SetInt(std::string name, int x) { m_pIntOptions[name.c_str()] = x; }
	void SetFloat(std::string name,float x) { m_pFloatOptions[name.c_str()]= x; }
	void SetString(std::string name,std::string x) { m_pStringOptions[name.c_str()]=x; }
	std::map<std::string, int> GetIntMap() { return m_pIntOptions; }
	std::map<std::string, float> GetFloatMap() { return m_pFloatOptions; }
	std::map<std::string, std::string> GetStrMap() { return m_pStringOptions; }
private:
	
	std::map<std::string, int> m_pIntOptions;
	std::map<std::string, float> m_pFloatOptions;
	std::map<std::string, std::string> m_pStringOptions;
};

class CD3D12Options
{
public:
	CD3D12Options();
	~CD3D12Options();
	void SaveCurrentSettings();
	void OpenSettingsFile();
	void CreateSettingsFile();
	std::wstring GetCurrentUpscaler() { return m_sCurrentUpScaler; }
	std::wstring GetCurrentDownscaler() { return m_sCurrentDownScaler; }
	std::wstring GetCurrentChromaUpscaler() { return m_sCurrentChromaUpscaler; }
	std::wstring GetCurrentImageDoubler() { return m_sCurrentImageDoubler; }
	void SetCurrentUpscaler(std::wstring x) 
	{ 
		m_sCurrentUpScaler = x; 
	}
	void SetCurrentDownscaler(std::wstring x) { m_sCurrentDownScaler = x; }
	void SetCurrentChromaUpscaler(std::wstring x) { m_sCurrentChromaUpscaler = x; }
	void SetCurrentImageDoubler(std::wstring x) { m_sCurrentImageDoubler = x; }
	void SetCurrentPostShader(std::wstring x) { m_sCurrentPostShader = x; }
	int GetCurrentDecoderBufferCount() { return m_iDecoderBufferCount; }
	int GetCurrentUploadBufferCount() { return m_iUploadBufferCount; }
	int GetCurrentRenderBufferCount() { return m_iRenderBufferCount; }
	int GetCurrentPresentBufferCount() { return m_iPresentBufferCount; }
	void SetCurrentDecoderBufferCount(int x) { m_iDecoderBufferCount = x; }
	void SetCurrentUploadBufferCount(int x) { m_iUploadBufferCount = x; }
	void SetCurrentRenderBufferCount(int x) { m_iRenderBufferCount = x; }
	void SetCurrentPresentBufferCount(int x) { m_iPresentBufferCount = x; }
	void SetScaler(std::string name, CScalerOption* opt)
	{
		for (std::vector<CScalerOption*>::iterator it = m_pOptions.begin(); it != m_pOptions.end(); it++)
		{
			if ((*it)->m_pScalerName == name)
			{
				*it = opt;
				return;
			}
		}

	}
	CScalerOption* GetScaler(const char* name)
	{
		for (std::vector<CScalerOption*>::iterator it = m_pOptions.begin(); it != m_pOptions.end(); it++)
		{
			if ((*it)->m_pScalerName == name)
				return *it;
		}
		return nullptr;
	}
private:
	std::vector<CScalerOption*> m_pOptions;
	std::string m_pFilePath;
	int m_iDecoderBufferCount;
	int m_iUploadBufferCount;
	int m_iRenderBufferCount;
	int m_iPresentBufferCount;

	std::wstring m_sCurrentUpScaler;
	std::wstring m_sCurrentDownScaler;
	std::wstring m_sCurrentChromaUpscaler;
	std::wstring m_sCurrentImageDoubler;
	std::wstring m_sCurrentPostShader;
};

