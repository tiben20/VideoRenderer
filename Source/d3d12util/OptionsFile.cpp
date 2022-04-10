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

#include "stdafx.h"
#include "OptionsFile.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <mutex>
#include <filesystem>
#include "utils/util.h"
#include <regex>



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
  std::regex word_regex("(.*?)=([^w]+)");
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

void CD3D12Options::SaveCurrentSettings()
{
  std::ofstream outfile;
  outfile.open(m_pFilePath.c_str());
  
  std::string currentline;
  currentline = "[currentupscaler]";  
  outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  currentline = "scalerid=";
  currentline.append(std::to_string(m_iCurrentUpScaler));
  outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  currentline = "[currentdownscaler]";  
  outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  currentline = "scalerid=";  
  currentline.append(std::to_string(m_iCurrentDownScaler));
  outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  currentline = "[currentchromascaler]";  
  outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  currentline = "scalerid=";
  currentline.append(std::to_string(m_iCurrentChromaUpscaler));
  outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  for (std::vector<CScalerOption*>::iterator it = m_pOptions.begin(); it != m_pOptions.end();it++)
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
        if (scaler == "currentupscaler")
        {
          std::getline(infile, line);
          option = GetOptionRegEx(line);
          m_iCurrentUpScaler = std::stoi(option.second);
          //got the currenetscaler get a new line and next
          std::getline(infile, line);
          goto scaler;
        }
        if (scaler == "currentdownscaler")
        {
          std::getline(infile, line);
          option = GetOptionRegEx(line);
          m_iCurrentDownScaler = std::stoi(option.second);
          //got the currenetscaler get a new line and next
          std::getline(infile, line);
          goto scaler;
        }
        if (scaler == "currentchromascaler")
        {
          std::getline(infile, line);
          option = GetOptionRegEx(line);
          m_iCurrentChromaUpscaler = std::stoi(option.second);
          //got the currenetscaler get a new line and next
          std::getline(infile, line);
          goto scaler;
        }
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
  if (optscaler)
    m_pOptions.push_back(optscaler);

}


//inline writelinetofile 
void CD3D12Options::CreateSettingsFile()
{
  std::ofstream outfile;
  outfile.open(m_pFilePath.c_str());
  std::string currentline;
  
  currentline="[currentupscaler]";  outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline="scalerid=6";  outfile.write(currentline.append("\n").c_str(), currentline.size()+1);
  currentline = "[currentdownscaler]";  outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  currentline = "scalerid=0";  outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  currentline = "[currentchromascaler]";  outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
  currentline = "scalerid=0";  outfile.write(currentline.append("\n").c_str(), currentline.size() + 1);
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


