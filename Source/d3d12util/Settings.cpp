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
  if (m_initialized)
    return false;
  m_pSettingsPath = getenv("APPDATA");
  m_pSettingsPath.append("\\TBD12\\settings.xml");
  if (!std::filesystem::exists(m_pSettingsPath.c_str()))
  {
    CreateSettingsFile();
    DLog(L"creating file for settings");

  }
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
        if (isInt(strSettingValue,&ivalue))
        {
          m_pSettingsMap.insert({ strSettingId,ivalue });
          goto nextsetting;
        }
        if (isDouble(strSettingValue,&dvalue))
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
  
  
  return false;
}

bool CSettings::Load()
{
  return false;
}

bool CSettings::Save()
{
  return false;
}

bool CSettings::Load(const std::string& file)
{
  return false;
}

bool CSettings::Load(const tinyxml2::XMLElement* root)
{
  return false;
}

bool CSettings::Save(const std::string& file)
{
  return false;
}

bool CSettings::Save(tinyxml2::XMLNode* root) const
{
  return false;
}

bool CSettings::LoadSetting(const tinyxml2::XMLNode* node, const std::string& settingId)
{
  return false;
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
  //std::map<std::string, std::variant<int, bool, double, std::string>>::const_iterator it;
  SettingsIterator = m_pSettingsMap.find(id);
  if (SettingsIterator == m_pSettingsMap.end())
  {
    DLog(L"didn't find the settings");
    m_pSettingsMap.insert({id,value});
    return;
  }
  if ((*SettingsIterator).second.index() == 1) {
    (*SettingsIterator).second = value;
    return;
    //std::get<bool>((*SettingsIterator).second) = value;
  }
  else
  {
    DLog(L"settings is not a bool");
  }
}

bool CSettings::ToggleBool(const std::string& id)
{
  return false;
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

bool CSettings::SetInt(const std::string& id, int value)
{
  return false;
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

bool CSettings::SetNumber(const std::string& id, double value)
{
  return false;
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
    return false;
  }
}

bool CSettings::SetString(const std::string& id, const std::string& value)
{
  return false;
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
  return false;
}



bool CSettings::CreateSettingsFile()
{
  return false;
}
