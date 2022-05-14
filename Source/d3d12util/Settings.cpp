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
#include "settings.h"
#include <filesystem>
#include "utils/Util.h"
#include "Utils/StringUtil.h"
#include "FileUtility.h"
using namespace tinyxml2;

CSettings::CSettings()
{
  
  Initialize();
}

CSettings::~CSettings()
{

}

CSettings& CSettings::GetInstance()
{
  static CSettings sDisplaySettings;
  return sDisplaySettings;
}

bool CSettings::Initialize()
{
  CAutoLock Lock(&m_critical);
  
  if (m_initialized)
    return false;
  m_pSettingsPath = getenv("APPDATA");
  m_pSettingsPath.append("\\TBD12\\settings.xml");

  if (!std::filesystem::exists(m_pSettingsPath.c_str()))
  {
    CreateSettingsFile();
    DLog(L"creating file for settings");
    //Already initialized dont need to load
    return true;
  }
  return Load();
}


bool CSettings::Save()
{
  tinyxml2::XMLDocument* xmldoc;
  xmldoc = new tinyxml2::XMLDocument();
  
  tinyxml2::XMLElement* root;
  root = xmldoc->NewElement("settings");
  
  //root->SetValue("settings");
  
  tinyxml2::XMLElement* curelement;
  for (SettingsIterator = m_pSettingsMap.begin(); SettingsIterator != m_pSettingsMap.end(); SettingsIterator++) {
    curelement = xmldoc->NewElement("setting");
    //curelement->SetValue("setting");
    
    curelement->SetAttribute("id", SettingsIterator->first.c_str());
    if (SettingsIterator->second.index() == 0) {
      //int
      curelement->SetText(std::get<int>(SettingsIterator->second));
    }
    else if (SettingsIterator->second.index() == 1) {
    //bool
      curelement->SetText(std::get<bool>(SettingsIterator->second));
    }
    else if (SettingsIterator->second.index() == 2) {
    //double
      curelement->SetText(std::get<double>(SettingsIterator->second));
    }
    else if (SettingsIterator->second.index() == 3) {
    //string
      curelement->SetText(std::get<std::string>(SettingsIterator->second).c_str());
    }
    root->InsertEndChild(curelement);
    //root->InsertFirstChild(curelement);
  
  
  }
  xmldoc->InsertFirstChild(root);
  //curelement->SetAttribute("id", )

  xmldoc->SaveFile(m_pSettingsPath.c_str());

  return true;
}

bool CSettings::Load()
{
  std::string settingstext;
  tinyxml2::XMLDocument* xmldoc;
  std::string attrvalue;
  std::string strSettingId;
  std::string strSettingValue;
  const tinyxml2::XMLAttribute* xmlattr;
  const tinyxml2::XMLElement* xmlelement;
  int ivalue = 0;
  bool bvalue = false;
  double dvalue = 0.0;
  std::string svalue = "";

  xmldoc = new tinyxml2::XMLDocument();
  XMLError res;
  if (Utility::ReadTextFile(m_pSettingsPath.c_str(), settingstext))
  {
    res = xmldoc->Parse(settingstext.c_str());
    if (res == XML_SUCCESS)
    {
      const tinyxml2::XMLElement* root = xmldoc->RootElement();
      attrvalue = root->Value();
      if (attrvalue._Equal("settings"))
      {
        xmlelement = root->FirstChildElement();
      anotherone:
        xmlattr = xmlelement->FindAttribute("id");
        strSettingId = xmlattr->Value();
        strSettingValue = xmlelement->GetText();
        str_tolower(strSettingId);
        str_tolower(strSettingValue);

        if (isBool(strSettingValue, &bvalue))
        {
          m_pSettingsMap.insert({ strSettingId,bvalue });
          goto nextsetting;
        }
        if (isInt(strSettingValue, &ivalue))
        {
          m_pSettingsMap.insert({ strSettingId,ivalue });
          goto nextsetting;
        }
        if (isDouble(strSettingValue, &dvalue))
        {
          m_pSettingsMap.insert({ strSettingId,dvalue });
          goto nextsetting;
        }
        m_pSettingsMap.insert({ strSettingId,strSettingValue });
      nextsetting:
        xmlelement = xmlelement->NextSiblingElement();
        if (xmlelement)
          goto anotherone;

      }


    }

  }
  else
  {
    DLog(L"error loading reintialising and saving");
    SetDefault();
    return Save();
  }
  return true;
}

void CSettings::SetDefault()
{
  SetBool(SETTING_RENDERER_DOUBLERATE, false);
  SetString(SETTING_RENDERER_UPSCALER,"bicubic");
  SetString(SETTING_RENDERER_DOWNSCALER, "bicubic");
  SetString(SETTING_RENDERER_CHROMAUPSAMPLER, "bicubic");
  SetInt(SETTING_RENDERER_SWAPEFFECT, RENDERER_SWAPMETHOD_FLIP);
  SetBool(SETTING_RENDERER_EXCLUSIVEFULLSCREEN, false);
  SetBool(SETTING_RENDERER_WAITVBLANK, true);
  SetBool(SETTING_RENDERER_SHOWSTATS, false);
  SetBool(SETTING_RENDERER_USEDITHER, false);
  SetBool(SETTING_RENDERER_HDR_PASSTHROUGH, false);
  SetBool(SETTING_RENDERER_HDR_TOGGLEDISPLAY, false);
  SetBool(SETTING_RENDERER_CONVERTTOSDR, false);
  SetInt(SETTING_RENDERER_TEXTURE_FORMAT, RENDERER_TEXFMT_AUTOINT);
  SetBool(SETTING_RENDERER_RESIZESTATS, false);
  SetBool(SETTING_RENDERER_INTERPORLATEAT50PCT, false);

}

bool CSettings::GetBool(const std::string& id) const
{
  std::map<std::string, std::variant<int, bool, double, std::string>>::const_iterator it;
  it = m_pSettingsMap.find(id);
  if (it == m_pSettingsMap.end())
  {
    DLog(L"didn't find the settings");
    return false;
  }
  if ((*it).second.index() == 1)
    return std::get<bool>((*it).second);
  else
  {
    DLog(L"settings is not a bool");
    return false;
  }
}

void CSettings::SetBool(const std::string& id, bool value)
{
  CAutoLock Lock(&m_critical);
  //std::map<std::string, std::variant<int, bool, double, std::string>>::const_iterator it;
  SettingsIterator = m_pSettingsMap.find(id);
  if (SettingsIterator == m_pSettingsMap.end())
  {
    m_pSettingsMap.insert({id,value});
    return;
  }
  if ((*SettingsIterator).second.index() == 1) {
    (*SettingsIterator).second = value;
    return;
  }
  else
    DLog(L"settings is not a bool");
}

bool CSettings::ToggleBool(const std::string& id)
{
  CAutoLock Lock(&m_critical);
  bool curvalue = false;
  SettingsIterator = m_pSettingsMap.find(id);
  if (SettingsIterator == m_pSettingsMap.end())
  {
    DLog(L"didn't find the settings toggle to true");
    m_pSettingsMap.insert({ id,true });
    return true;
  }
  if ((*SettingsIterator).second.index() == 1) {
    curvalue = std::get<bool>((*SettingsIterator).second);
    (*SettingsIterator).second = !curvalue;
    return curvalue;
  }
  else
    DLog(L"settings is not a bool");
  return curvalue;
}

int CSettings::GetInt(const std::string& id) const
{
  std::map<std::string, std::variant<int, bool, double, std::string>>::const_iterator it;
  it = m_pSettingsMap.find(id);
  if (it == m_pSettingsMap.end())
  {
    DLog(L"didn't find the settings");
    return false;
  }
  if ((*it).second.index() == 0)
    return std::get<int>((*it).second);
  else
  {
    DLog(L"settings is not a int");
    return 0;
  }
}

void CSettings::SetInt(const std::string& id, int value)
{
  CAutoLock Lock(&m_critical);
  SettingsIterator = m_pSettingsMap.find(id);
  if (SettingsIterator == m_pSettingsMap.end())
  {
    m_pSettingsMap.insert({ id,value });
    return;
  }
  if ((*SettingsIterator).second.index() == 0) {
    (*SettingsIterator).second = value;
    return;
  }
  else
    DLog(L"settings is not a bool");
}

double CSettings::GetNumber(const std::string& id) const
{
  std::map<std::string, std::variant<int, bool, double, std::string>>::const_iterator it;
  it = m_pSettingsMap.find(id);
  if (it == m_pSettingsMap.end())
  {
    DLog(L"didn't find the settings");
    return false;
  }
  if ((*it).second.index() == 2)
    return std::get<double>((*it).second);
  else
  {
    DLog(L"settings is not a float");
    return false;
  }
}

void CSettings::SetNumber(const std::string& id, double value)
{
  CAutoLock Lock(&m_critical);
  SettingsIterator = m_pSettingsMap.find(id);
  if (SettingsIterator == m_pSettingsMap.end())
  {
    m_pSettingsMap.insert({ id,value });
    return;
  }
  if ((*SettingsIterator).second.index() == 2) {
    (*SettingsIterator).second = value;
    return;
  }
  else
    DLog(L"settings is not a bool");
}

std::string CSettings::GetString(const std::string& id) const
{
  std::map<std::string, std::variant<int, bool, double, std::string>>::const_iterator it;
  it = m_pSettingsMap.find(id);
  if (it == m_pSettingsMap.end())
  {
    DLog(L"didn't find the settings");
    return false;
  }
  if ((*it).second.index() == 3)
    return std::get<std::string>((*it).second);
  else
  {
    DLog(L"settings is not a string");
    return "";
  }
}

void CSettings::SetString(const std::string& id, const std::string& value)
{
  CAutoLock Lock(&m_critical);
  SettingsIterator = m_pSettingsMap.find(id);
  if (SettingsIterator == m_pSettingsMap.end())
  {
    m_pSettingsMap.insert({ id,value });
    return;
  }
  if ((*SettingsIterator).second.index() == 3) {
    (*SettingsIterator).second = value;
    return;
  }
  else
    DLog(L"settings is not a bool");
}

bool CSettings::Load(const tinyxml2::XMLElement* root, bool& updated)
{
  return false;
}

bool CSettings::Load(const tinyxml2::XMLNode* settings)
{
  return false;
}

bool CSettings::Initialize(const std::string& file)
{
  return false;
}

bool CSettings::Reset()
{
  SetDefault();
  return true;
}



bool CSettings::CreateSettingsFile()
{
  SetDefault();
  return Save();
}
