/*
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
#include "GeometryRenderer.h"
#include "Texture.h"

#include "GraphicsCommon.h"
#include "CommandContext.h"
#include "PipelineState.h"
#include "RootSignature.h"
#include "BufferManager.h"
#include "d3d12util/CompiledShaders/GeometryPS.h"
#include "d3d12util/CompiledShaders/GeometryVS.h"
#include <map>
#include <string>
#include <cstdio>
#include <memory>
#include <malloc.h>

using namespace D3D12Engine;
using namespace Math;
using namespace std;

namespace GeometryRenderer
{
  RootSignature s_RootSignature;
  // handler sd and hd format 0: R8G8B8A8_UNORM   1: R11G11B10_FLOAT
  GraphicsPSO s_RectanglePSO[2] = { { L"Geometry Renderer: R8G8B8A8_UNORM PSO" },{ L"Geometry Renderer: R11G11B10_FLOAT PSO" } };
}

void GeometryRenderer::Initialize(void)
{
  s_RootSignature.Reset(0,1);
  s_RootSignature.InitStaticSampler(0, SamplerGeometryDesc, D3D12_SHADER_VISIBILITY_PIXEL);
  s_RootSignature.Finalize(L"GeometryRenderer", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
  D3D12_INPUT_ELEMENT_DESC vertElem[] =
  {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,  0 },
        { "COLOR",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 }
  };
  //sdr
  s_RectanglePSO[0].SetRootSignature(s_RootSignature);
  s_RectanglePSO[0].SetRasterizerState(D3D12Engine::RasterizerDefaultCw);
  s_RectanglePSO[0].SetBlendState(D3D12Engine::BlendDisable);
  s_RectanglePSO[0].SetDepthStencilState(D3D12Engine::DepthStateDisabled);
  s_RectanglePSO[0].SetInputLayout(_countof(vertElem),vertElem);
  s_RectanglePSO[0].SetSampleMask(0xFFFFFFFF);
  s_RectanglePSO[0].SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  s_RectanglePSO[0].SetVertexShader(g_pGeometryVS, sizeof(g_pGeometryVS));
  s_RectanglePSO[0].SetPixelShader(g_pGeometryPS, sizeof(g_pGeometryPS));
  s_RectanglePSO[0].SetRenderTargetFormats(1, &g_OverlayBuffer.GetFormat(), DXGI_FORMAT_UNKNOWN);
  s_RectanglePSO[0].Finalize();
  //hdr
  s_RectanglePSO[1] = s_RectanglePSO[0];
  s_RectanglePSO[1].SetRenderTargetFormats(1, &g_SceneColorBuffer.GetFormat(), DXGI_FORMAT_UNKNOWN);
  s_RectanglePSO[1].Finalize();
}

void GeometryRenderer::Shutdown(void)
{
  s_RootSignature.Free();
  s_RectanglePSO[0].FreePSO();
  s_RectanglePSO[1].FreePSO();
}

GeometryContext::GeometryContext(GraphicsContext& CmdContext)
  : m_Context(CmdContext)
{
  m_HDR = FALSE;
}

void GeometryContext::Begin(bool EnableHDR)
{
  m_HDR = (BOOL)EnableHDR;

  m_Context.SetRootSignature(GeometryRenderer::s_RootSignature);
  m_Context.SetPipelineState(GeometryRenderer::s_RectanglePSO[m_HDR]);
  m_Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}


void GeometryContext::End(void)
{
}

inline DirectX::XMFLOAT4 D3DCOLOR12toXMFLOAT4(const D3DCOLOR color)
{
  return DirectX::XMFLOAT4{
    (float)((color & 0x00FF0000) >> 16) / 255,
    (float)((color & 0x0000FF00) >> 8) / 255,
    (float)(color & 0x000000FF) / 255,
    (float)((color & 0xFF000000) >> 24) / 255,
  };
}

HRESULT GeometryContext::SetupVertex(const float x1, const float y1, const float x2, const float y2, const float x3, const float y3, const float x4, const float y4, const D3DCOLOR color)
{
  HRESULT hr = S_OK;

  m_bAlphaBlend = (color >> 24) < 0xFF;
  DirectX::XMFLOAT4 colorRGBAf = D3DCOLOR12toXMFLOAT4(color);
  m_Vertices[0] = { {x4, y4, 0.5f}, colorRGBAf };
  m_Vertices[1] = { {x1, y1, 0.5f}, colorRGBAf };
  m_Vertices[2] = { {x3, y3, 0.5f}, colorRGBAf };
  m_Vertices[3] = { {x2, y2, 0.5f}, colorRGBAf };

  return S_OK;
}

void GeometryContext::DrawRectangle(const RECT& rect, const SIZE& rtSize, const D3DCOLOR color)
{
  const float left = (float)(rect.left * 2) / rtSize.cx - 1;
  const float top = (float)(-rect.top * 2) / rtSize.cy + 1;
  const float right = (float)(rect.right * 2) / rtSize.cx - 1;
  const float bottom = (float)(-rect.bottom * 2) / rtSize.cy + 1;
  SetupVertex(left, top, right, top, right, bottom, left, bottom, color);
  m_Context.SetRootSignature(GeometryRenderer::s_RootSignature);
  m_Context.SetPipelineState(GeometryRenderer::s_RectanglePSO[0]);
  m_Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

  D3D12_VIEWPORT VP;
  VP.TopLeftX = 0;
  VP.TopLeftY = 0;
  VP.Width = rtSize.cx;
  VP.Height = rtSize.cy;
  VP.MinDepth = 0.0f;
  VP.MaxDepth = 1.0f;
  m_Context.SetViewport(VP);
  m_Context.SetDynamicVB(0, 4, sizeof(POINTVERTEX12), m_Vertices);
  
  m_Context.TransitionResource(g_OverlayBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
  m_Context.Draw(4,0);
  m_Context.TransitionResource(g_OverlayBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
}