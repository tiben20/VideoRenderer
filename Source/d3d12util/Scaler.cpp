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
#include "CompiledShaders/ScreenQuadPresentVS.h"

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

const DXGI_FORMAT DXGI_FORMAT_MAPPING[16] = {
  DXGI_FORMAT_R8_UNORM,
  DXGI_FORMAT_R16_UNORM,
  DXGI_FORMAT_R16_FLOAT,
  DXGI_FORMAT_R8G8_UNORM,
  DXGI_FORMAT_B5G6R5_UNORM,
  DXGI_FORMAT_R16G16_UNORM,
  DXGI_FORMAT_R16G16_FLOAT,
  DXGI_FORMAT_R8G8B8A8_UNORM,
  DXGI_FORMAT_B8G8R8A8_UNORM,
  DXGI_FORMAT_R10G10B10A2_UNORM,
  DXGI_FORMAT_R32_FLOAT,
  DXGI_FORMAT_R11G11B10_FLOAT,
  DXGI_FORMAT_R32G32_FLOAT,
  DXGI_FORMAT_R16G16B16A16_UNORM,
  DXGI_FORMAT_R16G16B16A16_FLOAT,
  DXGI_FORMAT_R32G32B32A32_FLOAT
};

void CD3D12Scaler::CreateDynTexture(std::wstring texture, CRect rect, DXGI_FORMAT fmt)
{
  ColorBuffer buf;
  if (m_pScalingTextureDyn.size() == 0)
    m_pScalingTextureDyn.push_back(buf);
  buf.Create(texture, rect.Width(), rect.Height(), 1, fmt);
  m_pScalingTextureDyn.push_back(buf);

  
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

void CD3D12Scaler::FreeDynTexture()
{
  for (std::vector<ColorBuffer>::iterator it = m_pScalingTextureDyn.begin(); it != m_pScalingTextureDyn.end(); it++)
  {
    it->DestroyBuffer();
  }

}

void CD3D12Scaler::SetDynRenderTargets(GraphicsContext& Context, std::vector<UINT> targets, bool setResourceState)
{

  std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> RTVs;
  RTVs.resize(targets.size());
  for (UINT x = 0; x < targets.size(); x++)
  {
    if (setResourceState)
      Context.TransitionResource(m_pScalingTextureDyn[targets[x]], D3D12_RESOURCE_STATE_RENDER_TARGET);
    RTVs.at(x) = m_pScalingTextureDyn[targets[x]].GetRTV();
  }

  //RTVs[0] = m_pScalingTexture[targets.at(0)].GetRTV();
  //RTVs[1] = m_pScalingTexture[targets.at(1)].GetRTV();
  Context.SetRenderTargets(targets.size(), RTVs.data());
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

void CD3D12Scaler::SetDynTextureSrv(GraphicsContext& Context, std::vector<UINT> idx, int root_index, bool setResourceState)
{
  std::vector< D3D12_CPU_DESCRIPTOR_HANDLE> phandle;
  int curidx = 0;
  for (UINT x : idx)
  {
    //phandle.push_back(m_pScalingTextureDyn[x].GetSRV());
    Context.TransitionResource(m_pScalingTextureDyn[x], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context.SetDynamicDescriptor(0, curidx, m_pScalingTextureDyn[x].GetSRV());
    //Context.SetDynamicDescriptor(root_index,curidx, m_pScalingTextureDyn[x].GetSRV());
    curidx += 1;
  }

  //Context.SetDynamicDescriptors(0, 0, phandle.size(), phandle.data());
  
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

CD3D12DynamicScaler::CD3D12DynamicScaler(std::wstring filename,bool *res)
{
  CShaderFileLoader* shader;
  shader = new CShaderFileLoader(filename);
  m_pDesc = {};
  res = (bool*)shader->Compile(m_pDesc);
  m_pScaler = new CD3D12Scaler(L"DynamicScaler");
}

void CD3D12DynamicScaler::Init(DXGI_FORMAT srcfmt,CRect src,CRect dst)
{
  GraphicsPSO pso;
  m_srcRect = src;

  for (ShaderIntermediateTextureDesc x : m_pDesc.textures)
  {
    CRect texsize = src;
    if (x.name != "INPUT")
    {
      if (x.sizeExpr.first == "INPUT_WIDTH")
      {
        texsize.right = src.right;
      }
      if (x.sizeExpr.second == "INPUT_HEIGHT")
      {
        texsize.bottom = src.bottom;
      }
      std::wstring currenttex(x.name.begin(), x.name.end());
      
      m_pScaler->CreateDynTexture(currenttex,src, DXGI_FORMAT_MAPPING[(int)x.format]);
    }
    
  }
  for (ShaderPassDesc i : m_pDesc.passes)
  {
    GraphicsPSO pso;
    
    pso.SetRootSignature(D3D12Engine::g_RootScalers);
    pso.SetRasterizerState(D3D12Engine::RasterizerDefault);
    pso.SetBlendState(D3D12Engine::Blendfxrcnnx);
    pso.SetDepthStencilState(D3D12Engine::DepthStateDisabled);
    pso.SetSampleMask(0xFFFFFFFF);
    pso.SetInputLayout(0, nullptr);
    
    pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    pso.SetVertexShader(g_pScreenQuadPresentVS, sizeof(g_pScreenQuadPresentVS));
    pso.SetPixelShader(i.cso->GetBufferPointer(), i.cso->GetBufferSize());
    DXGI_FORMAT formats[4];
    for (UINT idx = 0; idx < i.outputs.size(); idx++)
    {
      formats[idx] = m_pScaler->GetDynTexture(i.outputs.at(idx)).GetFormat();
    }
    if (i.outputs.size() == 0)
      pso.SetRenderTargetFormat(srcfmt, DXGI_FORMAT_UNKNOWN);
    else
      pso.SetRenderTargetFormats(i.outputs.size(), formats, DXGI_FORMAT_UNKNOWN);

     //= { DXGI_FORMAT_B8G8R8A8_UNORM ,DXGI_FORMAT_R16G16B16A16_FLOAT ,srcfmt };
    
    pso.Finalize();
    m_pPSO.push_back(pso);
  }
}

void CD3D12DynamicScaler::Render(GraphicsContext& Context,CRect dstrect, ColorBuffer& dest, ColorBuffer& source)
{
  //1.0f / (float)source.GetWidth();
  //1.0f / (float)source.GetHeight();
  Context.SetRootSignature(D3D12Engine::g_RootScalers);
  Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  Context.SetViewportAndScissor(0, 0, source.GetWidth(), source.GetHeight());
  int passes = 0;
  std::vector<DWParam> constvars;
  DWParam inputx = (float)1.0f / (float)m_srcRect.Width();
  DWParam inputy = (float)1.0f / (float)m_srcRect.Height();

  Context.SetDynamicDescriptor(0, 0, source.GetSRV());
  for (ShaderPassDesc i : m_pDesc.passes)
  {
    //set shader resource view
    if (i.inputs.size() != 0 && i.inputs.at(0) != 0)
      m_pScaler->SetDynTextureSrv(Context,i.inputs,0,true);
      
    //set render target
    if (i.outputs.size() != 0)
      m_pScaler->SetDynRenderTargets(Context, i.outputs,true);
    else
    {
      //last pass so set viewport and output render target
      Context.SetRenderTarget(dest.GetRTV());
      Context.SetViewportAndScissor(dstrect.left, dstrect.top, dstrect.Width(), dstrect.Height());
    }
    //set the passes pipeline
    Context.SetPipelineState(m_pPSO[passes]);

    // set constant buffer
    Context.SetConstants(1, inputx, inputy);
    Context.Draw(3);
    passes += 1;
  }
  
  
}

void CD3D12DynamicScaler::Unload()
{
  for (GraphicsPSO i : m_pPSO)
  {
    i.FreePSO();
    i.DestroyAll();
  }
  m_pScaler->FreeDynTexture();
  m_pScaler = nullptr;
}
CD3D12DynamicScaler::~CD3D12DynamicScaler()
{
  Unload();
}