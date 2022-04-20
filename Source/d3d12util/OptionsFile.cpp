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
#include "OptionsFile.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <mutex>
#include <filesystem>
#include "utils/util.h"
#include <regex>
#include "Utility.h"


void CScalerOption::AddInt(const char* name, std::string value)
{
  int valuei = std::stoi(value);
   m_pIntOptions.insert({ name,valuei });
}
void CScalerOption::AddFloat(const char* name, std::string value)
{
  float valuef = std::stof(value);
  m_pFloatOptions.insert({ name,valuef });
}


CD3D12Options::CD3D12Options()
{
  //switch to _dupenv_s
  m_pFilePath = getenv("APPDATA");
  
  m_pFilePath.append("\\MPCVideoRenderer");
  m_pFilePath.append("\\d3d12renderer.settings");
  if (!std::filesystem::exists(m_pFilePath.c_str()))
  {
    CreateSettingsFile();
    DLog(L"creating file for settings");

  }
  OpenSettingsFile();
}

CD3D12Options::~CD3D12Options()
{
  DLog(L"~CD3D12Options");
}
std::string GetScalerRegEx(std::string str)
{

  //std::regex self_regex("\\[(.*?)\\]",
  //  std::regex_constants::ECMAScript | std::regex_constants::icase);
  
  std::regex word_regex("\\[(.*?)\\]");
  auto words_begin =
    std::sregex_iterator(str.begin(), str.end(), word_regex);
  auto words_end = std::sregex_iterator();
  
  for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
    
    std::smatch match = *i;
    std::string match_str = match[1].str();
    if (match_str.size()>0)
    {
      
      return match_str;
    }
  }

  return "";
}

std::pair<std::string,std::string> GetOptionRegEx(std::string str)
{
  //^([^=\r\n]+)=(.*) would be good too
  std::regex word_regex("(.*?)=([^$]+)");
  auto words_begin =
    std::sregex_iterator(str.begin(), str.end(), word_regex);
  auto words_end = std::sregex_iterator();

  for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
    std::smatch match = *i;
    std::string match_str = match.str();
    if (match_str.size() > 0)
    {
      return { match[1],match[2] };
    }
  }

  return { "","" };
}
/*
[POST]
scaler=Bicubic.hlsl
paramB=0.333333
paramC=0.333333
scaler=SSimDownscaler.hlsl
variant=0*/
void CD3D12Options::SaveCurrentSettings()
{
  std::ofstream outfile;
  outfile.open(m_pFilePath.c_str());
  
  std::string currentline;
  currentline = "[renderersettings]";
  outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  currentline = "currentupscaler=";
  currentline.append(Utility::WideStringToUTF8(m_sCurrentUpScaler));
  currentline += "\ncurrentchromascaler=";  
  currentline.append(Utility::WideStringToUTF8(m_sCurrentDownScaler));
  currentline += "\ncurrentdownscaler=";
  currentline.append(Utility::WideStringToUTF8(m_sCurrentChromaUpscaler));
  currentline += "\ncurrentimagedoubler=";
  currentline.append(Utility::WideStringToUTF8(m_sCurrentImageDoubler));
  currentline += "\ndecoderbuffercount=";
  currentline.append(std::to_string(m_iDecoderBufferCount));
  currentline += "\nuploadbuffercount=";
  currentline.append(std::to_string(m_iUploadBufferCount));
  currentline += "\nrenderbuffercount=";
  currentline.append(std::to_string(m_iRenderBufferCount));
  currentline += "\npresentbuffercount=";
  currentline.append(std::to_string(m_iPresentBufferCount));
  outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  for (std::vector<CScalerOption*>::iterator it = m_pOptions.begin(); it != m_pOptions.end(); it++)
  {
    //only write to options if it has more than the scaler type
    if (((*it)->GetFloatMap().size() + (*it)->GetIntMap().size() + (*it)->GetStrMap().size()) > 1)
    {
      currentline = "[";
      currentline.append((*it)->m_pScalerName).append("]").append("\n");
      outfile.write(currentline.c_str(), currentline.size());
      std::map<std::string, std::string> strmap = (*it)->GetStrMap();
      for (std::map<std::string, std::string>::iterator itt = strmap.begin(); itt != strmap.end(); itt++)
      {
        currentline = (*itt).first;
        currentline.append("=");
        currentline.append((*itt).second).append("\n");
        outfile.write(currentline.c_str(), currentline.size());
      }
      std::map<std::string, int> intmap = (*it)->GetIntMap();
      for (std::map<std::string, int>::iterator itt = intmap.begin(); itt != intmap.end(); itt++)
      {
        currentline = (*itt).first;
        currentline.append("=");
        currentline.append(std::to_string((*itt).second)).append("\n");
        outfile.write(currentline.c_str(), currentline.size());
      }
      std::map<std::string, float> floatmap = (*it)->GetFloatMap();
      for (std::map<std::string, float>::iterator itt = floatmap.begin(); itt != floatmap.end(); itt++)
      {
        currentline = (*itt).first;
        currentline.append("=");
        currentline.append(std::to_string((*itt).second)).append("\n");
        outfile.write(currentline.c_str(), currentline.size());
      }
    }
  }
  if (m_pPostScalerOptions.size() > 0)
  {
    currentline = "[POST]\n";
    outfile.write(currentline.c_str(), currentline.size());
  }
  for (std::vector<CScalerOption*>::iterator it = m_pPostScalerOptions.begin(); it != m_pPostScalerOptions.end(); it++)
  {
    
    currentline = "scaler=";
    currentline.append((*it)->m_pScalerName);
    currentline.append("\n");
    outfile.write(currentline.c_str(), currentline.size());
    std::map<std::string, std::string> strmap = (*it)->GetStrMap();
    for (std::map<std::string, std::string>::iterator itt = strmap.begin(); itt != strmap.end(); itt++)
    {
      if ((*itt).first != "type" && (*itt).first != "scaler" && (*itt).first != "new")
      {
        currentline = (*itt).first;
        currentline.append("=");
        currentline.append((*itt).second).append("\n");
        outfile.write(currentline.c_str(), currentline.size());
      }
    }
    std::map<std::string, int> intmap = (*it)->GetIntMap();
    for (std::map<std::string, int>::iterator itt = intmap.begin(); itt != intmap.end(); itt++)
    {
      currentline = (*itt).first;
      currentline.append("=");
      currentline.append(std::to_string((*itt).second)).append("\n");
      outfile.write(currentline.c_str(), currentline.size());
    }
    std::map<std::string, float> floatmap = (*it)->GetFloatMap();
    for (std::map<std::string, float>::iterator itt = floatmap.begin(); itt != floatmap.end(); itt++)
    {
      currentline = (*itt).first;
      currentline.append("=");
      currentline.append(std::to_string((*itt).second)).append("\n");
      outfile.write(currentline.c_str(), currentline.size());
    }
     
  }
  outfile.close();
}

void CD3D12Options::OpenSettingsFile()
{
  std::ifstream infile(m_pFilePath.c_str());
  std::string line;
  std::string scaler;
  std::pair<std::string,std::string> option;
  char const* digits = "0123456789";
  CScalerOption* optscaler = nullptr;
  if (infile.is_open())
  {
    while (std::getline(infile, line))
    {
      std::istringstream iss(line);
    scaler:
      if (optscaler)
      {
        m_pOptions.push_back(optscaler);
        optscaler = nullptr;
      }
      if (line.find("[") == 0)
      {
        scaler = GetScalerRegEx(line);
      }
      if (scaler.size() > 0)
      {
        if (scaler.find("renderersettings") != std::string::npos)
        {
          while (std::getline(infile, line))
          {
            if (line.find("[") == 0)
            {
              scaler = GetScalerRegEx(line);
              goto scaler;
            }

            option = GetOptionRegEx(line);
            if (option.first.find("currentupscaler") != std::string::npos)
              m_sCurrentUpScaler = Utility::UTF8ToWideString(option.second);
            if (option.first.find("currentdownscaler") != std::string::npos)
              m_sCurrentDownScaler = Utility::UTF8ToWideString(option.second);
            if (option.first.find("currentchromascaler") != std::string::npos)
              m_sCurrentChromaUpscaler = Utility::UTF8ToWideString(option.second);
            if (option.first.find("currentimagedoubler") != std::string::npos)
              m_sCurrentImageDoubler = Utility::UTF8ToWideString(option.second);
            if (option.first.find("decoderbuffercount") != std::string::npos)
              m_iDecoderBufferCount = std::stoi(option.second);
            if (option.first.find("uploadbuffercount") != std::string::npos)
              m_iUploadBufferCount = std::stoi(option.second);
            if (option.first.find("renderbuffercount") != std::string::npos)
              m_iRenderBufferCount = std::stoi(option.second);
            if (option.first.find("presentbuffercount") != std::string::npos)
              m_iPresentBufferCount = std::stoi(option.second);
            //got the currenetscaler get a new line and next
          }
          goto scaler;
        }
        if (scaler == "POST")
        {
          //post scalers
          optscaler = nullptr;
          while (std::getline(infile, line))
          {
            
            option = GetOptionRegEx(line);
            //we hit header []
            if (option.first.size() == 0)
            {
              //fill and go to next
              if (optscaler)
                m_pPostScalerOptions.push_back(optscaler);
              goto scaler;
            }
            if (option.first == "scaler")
            {
              //if scaler not null we already filled one earlier
              if (optscaler)
                m_pPostScalerOptions.push_back(optscaler);
              optscaler = new CScalerOption(option.second);
              optscaler->SetString("type", "POST");
            }
            if (option.second.find_first_of(digits) != std::string::npos && option.second.find_first_of(".hlsl") == std::string::npos)
            {
              if (option.second.find(".") != std::string::npos)//float
                optscaler->AddFloat(option.first.c_str(), option.second.c_str());
              else
                optscaler->AddInt(option.first.c_str(), option.second.c_str());
            }
            else
            {
              optscaler->AddString(option.first.c_str(), option.second);
            }
          }
          if (optscaler)
          {
            m_pPostScalerOptions.push_back(optscaler);
            optscaler = nullptr;
          }
        }
        else
        {
          //unique scaler for types
          optscaler = new CScalerOption(scaler.c_str());
          while (std::getline(infile, line))
          {
            option = GetOptionRegEx(line);
            if (option.first.size() == 0)
              goto scaler;
            if (option.second.find_first_of(digits) == std::string::npos)
              optscaler->AddString(option.first.c_str(), option.second);
            else
            {
              if (option.second.find(".") != std::string::npos)//float
                optscaler->AddFloat(option.first.c_str(), option.second.c_str());
              else
                optscaler->AddInt(option.first.c_str(), option.second.c_str());
            }
          }
        }
      }

    }
  }
  if (optscaler)
    m_pOptions.push_back(optscaler);

}


//inline writelinetofile 
void CD3D12Options::CreateSettingsFile()
{
  std::ofstream outfile;
  outfile.open(m_pFilePath.c_str());
  std::string currentline;
  
  currentline="[renderersettings]";  outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="currentupscaler=2";  outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline = "currentchromascaler=0";  outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  currentline = "scalerid=0";  outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  currentline = "currentdownscaler=0";  outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  currentline = "decoderbuffercount=16";  outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  currentline = "uploadbuffercount=8";  outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  currentline = "renderbuffercount=8";  outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  currentline = "presentbuffercount=8";  outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  currentline="[bilinear]";  outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="[d3d12]";  outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="[cubic]"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="bicubic=3"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="[lanczos]"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="lanczostype=2"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="[spline]"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="splinetype=1"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="[jinc]"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline = "windowsinc=0"; outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  currentline = "sinc=0"; outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  currentline = "str=0"; outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  currentline="[superxbr]"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="factor=0"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="strength=5"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="sharp=0.000000"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="[fxrcnnx]"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="function=spline"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="passes=2"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="smoothness=0.200000"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="strength=0.500000"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="[superresxbr]"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="function=spline"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="factor=2"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="respasses=2"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="xbrstrength=2"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="ressmoothness=0.500000"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="resstrength=0.500000"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="xbrsharp=1.500000"; outfile.write(currentline.append("\n").c_str(), currentline.size()+1);


  outfile.close();
}


