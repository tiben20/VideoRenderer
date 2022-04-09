//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

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
	int GetCurrentUpscaler() { return m_iCurrentUpScaler; }
  int GetCurrentDownscaler() { return m_iCurrentDownScaler; }
	int GetCurrentChromaUpscaler() { return m_iCurrentChromaUpscaler; }
	void SetCurrentUpscaler(int x) { m_iCurrentUpScaler =x ; }
	void SetCurrentDownscaler(int x) { m_iCurrentDownScaler = x; }
	void SetCurrentChromaUpscaler(int x) { m_iCurrentChromaUpscaler = x; }
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
	int m_iCurrentUpScaler;
	int m_iCurrentDownScaler;
	int m_iCurrentChromaUpscaler;
};

