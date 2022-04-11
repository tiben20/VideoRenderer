/*
 * Original developed by Minigraph author James Stanard
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
#include <string>
#include <vector>
#include "commandcontext.h"
#include "ColorBuffer.h"

__declspec(align(16)) struct CONSTANT_BUFFER_4F_4int {
  DirectX::XMFLOAT4 size0;
  DirectX::XMFLOAT4 fArg;
  DirectX::XMINT4 iArg;
  DirectX::XMFLOAT2 fInternalArg;
  DirectX::XMINT2 iInternalArg;
};

struct ScalerConfigFloat
{
  std::wstring Name;
  float Value;
};

struct ScalerConfigInt
{
  std::wstring Name;
  int Value;
};


class CD3D12Scaler
{
public:
  CD3D12Scaler(std::wstring name) { m_pName = name; m_pConstantBuffer = {}; }
  ~CD3D12Scaler();
  

  std::wstring Name() { return m_pName; }

  std::vector<ScalerConfigFloat> g_ScalerInternalFloat;
  std::vector<ScalerConfigInt> g_ScalerInternalInt;
  /*void AddConfig(std::wstring Name, int Value, int MinValue = 0, int MaxValue = 0, int increment = 0)
  {
    m_pScalerConfigInt.push_back(ScalerConfigInt{ Name,Value,MinValue,MaxValue,increment });
  }
  void AddConfig(std::wstring Name, float Value, float MinValue = 0.0f, float MaxValue = 0.0f, float increment = 0.0f)
  {
    m_pScalerConfigFloat.push_back(ScalerConfigFloat{ Name,Value,MinValue,MaxValue,increment });
  }

  void AddBufferConstant(ScalerConfigInt cfg) { m_pScalerConfigInt.push_back(cfg); };
  void AddBufferConstant(ScalerConfigFloat cfg) { m_pScalerConfigFloat.push_back(cfg); };*/
  float GetConfigFloat(std::wstring name);
  int GetConfigInt(std::wstring name);
  void SetConfigFloat(std::wstring name, float value);
  void SetConfigInt(std::wstring name, int value);

  void ShaderPass(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, int w, int h, int iArgs[4], float fArgs[4]);
  void Done() { m_bFirstPass = true; }
  void CreateTexture(std::wstring name, CRect rect, DXGI_FORMAT fmt);
  void SetTextureSrv(GraphicsContext& Context, std::wstring name, int index, int table, bool setResourceState = true);
  void SetRenderTargets(GraphicsContext& Context, std::vector<std::wstring> targets, bool setResourceState=false);
  bool         g_bTextureCreated = false;
private:
  std::wstring m_pName;
  bool         m_bFirstPass = true;
  
  std::map<std::wstring,ColorBuffer> m_pScalingTexture;
  
  /*std::vector<ScalerConfigInt> m_pScalerConfigInt;
  std::vector<ScalerConfigFloat> m_pScalerConfigFloat;*/
  CONSTANT_BUFFER_4F_4int m_pConstantBuffer;
};