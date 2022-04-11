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

#include "stdafx.h"
#include "scaler.h"
#include "math/Common.h"
CD3D12Scaler::~CD3D12Scaler()
{
  
  DLog(L"d3dscaler exit");
  
}

DirectX::XMFLOAT4 CreateFloat4(int width, int height)
{
  float x = (float)width;
  float y = (float)height;
  float z = (float)1 / width;
  float w = (float)1 / height;
  return { x,y,z,w };
}

void CD3D12Scaler::CreateTexture(std::wstring name, CRect rect, DXGI_FORMAT fmt)
{
  ColorBuffer buf;
  buf.Create(name, rect.Width(), rect.Height(), 1, fmt);
  m_pScalingTexture.insert({ name,buf });

  
}

void CD3D12Scaler::FreeTexture()
{
  for (std::map<std::wstring,ColorBuffer>::iterator it = m_pScalingTexture.begin(); it != m_pScalingTexture.end(); it++)
  {
    it->second.DestroyBuffer();
  }

}
void CD3D12Scaler::SetRenderTargets(GraphicsContext& Context, std::vector<std::wstring> targets, bool setResourceState)
{
  
  std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> RTVs;
  RTVs.resize(targets.size());
  for (int x = 0; x < targets.size(); x++)
  {
    if (setResourceState)
      Context.TransitionResource(m_pScalingTexture[targets.at(x)], D3D12_RESOURCE_STATE_RENDER_TARGET);
    RTVs.at(x) = m_pScalingTexture[targets.at(x)].GetRTV();
  }

  //RTVs[0] = m_pScalingTexture[targets.at(0)].GetRTV();
  //RTVs[1] = m_pScalingTexture[targets.at(1)].GetRTV();
  Context.SetRenderTargets(targets.size(), RTVs.data());
}
void CD3D12Scaler::SetTextureSrv(GraphicsContext& Context, std::wstring name, int index, int table, bool setResourceState)
{
  Context.TransitionResource(m_pScalingTexture[name], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
  Context.SetDynamicDescriptor(index, table, m_pScalingTexture[name].GetSRV());
}

void CD3D12Scaler::ShaderPass(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, int w, int h, int iArgs[4],float fArgs[4])
{
  //pipeline state and root signature should already be set
  // just removed
  //Context.TransitionResource(dest, D3D12_RESOURCE_STATE_RENDER_TARGET);
  //Context.TransitionResource(source, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
  
  /*Context.SetRenderTarget(dest.GetRTV());
  if (m_bFirstPass)
  {
    Context.SetViewportAndScissor(0, 0, dest.GetWidth(), dest.GetHeight());
    Context.SetDynamicDescriptor(0, 0, source.GetSRV());
  }*/

  m_pConstantBuffer.size0 = CreateFloat4(w, h);
  m_pConstantBuffer.fArg.x = fArgs[0];
  m_pConstantBuffer.fArg.y = fArgs[1];
  m_pConstantBuffer.fArg.z = fArgs[2];
  m_pConstantBuffer.fArg.w = fArgs[3];
  m_pConstantBuffer.iArg.x = iArgs[0];
  m_pConstantBuffer.iArg.y = iArgs[1];
  m_pConstantBuffer.iArg.z = iArgs[2];
  m_pConstantBuffer.iArg.w = iArgs[3];
  m_pConstantBuffer.fInternalArg.x = g_ScalerInternalFloat.size() > 0 ? g_ScalerInternalFloat[0].Value : 0.0f;
  m_pConstantBuffer.fInternalArg.y = g_ScalerInternalFloat.size() > 0 ? g_ScalerInternalFloat[1].Value : 0.0f;
  m_pConstantBuffer.iInternalArg.x = g_ScalerInternalInt.size() > 0 ? g_ScalerInternalInt[0].Value : 0;
  m_pConstantBuffer.iInternalArg.y = g_ScalerInternalInt.size() > 1 ? g_ScalerInternalInt[1].Value : 0;
  //the dest become the source after the first pass
  /*if (!m_bFirstPass)
    Context.SetDynamicDescriptor(0, 0, dest.GetSRV());*/

  Context.SetDynamicConstantBufferView(1, sizeof(CONSTANT_BUFFER_4F_4int), &m_pConstantBuffer);
  Context.Draw(3);


  
  m_bFirstPass = false;
}

