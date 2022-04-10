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
#include "TextRenderer.h"
#include "Texture.h"

#include "GraphicsCommon.h"
#include "CommandContext.h"
#include "PipelineState.h"
#include "RootSignature.h"
#include "Helper.h"
#include "D3DUtil/FontBitmap.h"
#include "CompiledShaders/ps_font.h"
#include "CompiledShaders/vs_simple.h"
#define MAX_NUM_VERTS 400*6

struct Font12Vertex {
  DirectX::XMFLOAT3 Pos;
  DirectX::XMFLOAT2 Tex;
};

struct PixelBufferType12
{
  DirectX::XMFLOAT4 pixelColor;
};

inline auto Char2Index12(WCHAR ch)
{
  if (ch >= 0x0020 && ch <= 0x007F) {
    return ch - 32;
  }
  if (ch >= 0x00A0 && ch <= 0x00BF) {
    return ch - 64;
  }
  return 0x007F - 32;
}

using namespace D3D12Engine;
using namespace Math;
using namespace std;

namespace TextRenderer
{
  RootSignature s_RootSignature;
  // handler sd and hd format 0: R8G8B8A8_UNORM   1: R11G11B10_FLOAT
  GraphicsPSO s_FontPSO[2] = { { L"Geometry Renderer: R8G8B8A8_UNORM PSO" },{ L"Geometry Renderer: R11G11B10_FLOAT PSO" } };

  // Font properties
  std::wstring m_strFontName;
  UINT m_fontHeight = 0;
  UINT m_fontFlags = 0;

  WCHAR m_Characters[128];
  FloatRect m_fTexCoords[128] = {};
  SIZE m_MaxCharMetric = {};

  D3DCOLOR m_Color = D3DCOLOR_XRGB(255, 255, 255);

  //Textures
  UINT  m_uTexWidth = 0;
  UINT  m_uTexHeight = 0;
  Texture m_pTexture;
  
}

void TextRenderer::Initialize(void)
{
  s_RootSignature.Reset(2,1);
  D3D12_SAMPLER_DESC SampDesc = {};
  SampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
  SampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  SampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  SampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  SampDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  SampDesc.MinLOD = 0;
  SampDesc.MaxLOD = D3D12_FLOAT32_MAX;
  
  s_RootSignature.InitStaticSampler(0, SampDesc, D3D12_SHADER_VISIBILITY_PIXEL);
  s_RootSignature[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
  s_RootSignature[1].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_ALL);
  s_RootSignature.Finalize(L"TextRenderer", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
  D3D12_INPUT_ELEMENT_DESC vertElem[] =
  {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,  0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 }
  };
  //sdr
  s_FontPSO[0].SetRootSignature(s_RootSignature);
  s_FontPSO[0].SetRasterizerState(D3D12Engine::RasterizerDefaultCw);
  s_FontPSO[0].SetBlendState(D3D12Engine::BlendFont);
  s_FontPSO[0].SetDepthStencilState(D3D12Engine::DepthStateDisabled);
  s_FontPSO[0].SetInputLayout(_countof(vertElem),vertElem);
  s_FontPSO[0].SetSampleMask(0xFFFFFFFF);
  s_FontPSO[0].SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  s_FontPSO[0].SetVertexShader(g_pvs_simple, sizeof(g_pvs_simple));
  s_FontPSO[0].SetPixelShader(g_pps_font, sizeof(g_pps_font));
  s_FontPSO[0].SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_UNKNOWN);
  s_FontPSO[0].Finalize();
  //hdr
  s_FontPSO[1] = s_FontPSO[0]; 
  s_FontPSO[1].SetRenderTargetFormat(DXGI_FORMAT_R11G11B10_FLOAT, DXGI_FORMAT_UNKNOWN);
  s_FontPSO[1].Finalize();

  // initialize the character array
  UINT idx = 0;
  for (WCHAR ch = 0x0020; ch < 0x007F; ch++) {
    m_Characters[idx++] = ch;
  }
  m_Characters[idx++] = 0x25CA; // U+25CA Lozenge
  for (WCHAR ch = 0x00A0; ch <= 0x00BF; ch++) {
    m_Characters[idx++] = ch;
  }
  ASSERT(idx == std::size(m_Characters));

}

SIZE TextRenderer::GetMaxCharMetric()
{
  return m_MaxCharMetric;
}
HRESULT TextRenderer::LoadFont(const WCHAR* strFontName, const UINT fontHeight, const UINT fontFlags)
{
  if (fontHeight == m_fontHeight && fontFlags == m_fontFlags && m_strFontName.compare(strFontName) == 0) {
    return S_FALSE;
  }
  m_strFontName = strFontName;
  m_fontHeight = fontHeight;
  m_fontFlags = fontFlags;

  CFontBitmap fontBitmap;

  HRESULT hr = fontBitmap.Initialize(m_strFontName.c_str(), m_fontHeight, 0, m_Characters, std::size(m_Characters));
  if (FAILED(hr)) {
    return hr;
  }

  m_MaxCharMetric = fontBitmap.GetMaxCharMetric();
  m_uTexWidth = fontBitmap.GetWidth();
  m_uTexHeight = fontBitmap.GetHeight();
  EXECUTE_ASSERT(S_OK == fontBitmap.GetFloatCoords((FloatRect*)&m_fTexCoords, std::size(m_Characters)));

  BYTE* pData = nullptr;
  UINT uPitch = 0;
  hr = fontBitmap.Lock(&pData, uPitch);
  if (FAILED(hr)) {
    return hr;
  }

  m_pTexture.Create2D(m_uTexWidth*4, m_uTexWidth, m_uTexHeight, DXGI_FORMAT_B8G8R8A8_UNORM, pData);
  return S_OK;
}

void TextRenderer::Shutdown(void)
{
  s_RootSignature.Free();
  s_FontPSO[0].FreePSO();
  s_FontPSO[1].FreePSO();
  m_pTexture.Destroy();
  m_fontHeight = 0;
}

FontContext::FontContext(GraphicsContext& CmdContext)
  : m_Context(CmdContext)
{
  m_HDR = FALSE;
}

void FontContext::Begin(bool EnableHDR)
{
  m_HDR = (BOOL)EnableHDR;

  m_Context.SetRootSignature(TextRenderer::s_RootSignature);
  m_Context.SetPipelineState(TextRenderer::s_FontPSO[m_HDR]);
  m_Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}


void FontContext::End(void)
{
}

inline DirectX::XMFLOAT4 D3D12COLORtoXMFLOAT4(const D3DCOLOR color)
{
  return DirectX::XMFLOAT4{
    (float)((color & 0x00FF0000) >> 16) / 255,
    (float)((color & 0x0000FF00) >> 8) / 255,
    (float)(color & 0x000000FF) / 255,
    (float)((color & 0xFF000000) >> 24) / 255,
  };
}

void FontContext::DrawText(const SIZE& rtSize, float sx, float sy, D3DCOLOR color, const WCHAR* strText)
{
  HRESULT hr = S_OK;
  UINT Stride = sizeof(Font12Vertex);
  UINT Offset = 0;

  if (color != TextRenderer::m_Color) {
    TextRenderer::m_Color = color;
    DirectX::XMFLOAT4 colorRGBAf = D3D12COLORtoXMFLOAT4(TextRenderer::m_Color);
  }
  m_Context.SetRootSignature(TextRenderer::s_RootSignature);
  m_Context.SetPipelineState(TextRenderer::s_FontPSO[0]);
  m_Context.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  //m_Context.TransitionResource(g_OverlayBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);

  D3D12_VIEWPORT VP;
  VP.TopLeftX = 0;
  VP.TopLeftY = 0;
  VP.Width = rtSize.cx;
  VP.Height = rtSize.cy;
  VP.MinDepth = 0.0f;
  VP.MaxDepth = 1.0f;

  m_Context.SetViewport(VP);

  const float fStartX = (float)(sx * 2) / rtSize.cx - 1;
  const float fLineHeight = (TextRenderer::m_fTexCoords[0].bottom - TextRenderer::m_fTexCoords[0].top) * TextRenderer::m_uTexHeight * 2 / rtSize.cy;
  float drawX = fStartX;
  float drawY = (float)(-sy * 2) / rtSize.cy + 1;
  std::vector<Font12Vertex> m_vertices;

  CONSTANT_BUFFER_COLOR colorconstant;
  D3DCOLOR colll = D3DCOLOR_ARGB(255, 255, 255, 255);
  colorconstant.col = D3D12COLORtoXMFLOAT4(colll);

  while (*strText) {
    const WCHAR c = *strText++;

    if (c == '\n') {
      drawX = fStartX;
      drawY -= fLineHeight;
      continue;
    }

    const auto tex = TextRenderer::m_fTexCoords[Char2Index12(c)];

    const float Width = (tex.right - tex.left) * TextRenderer::m_uTexWidth * 2 / rtSize.cx;
    const float Height = (tex.bottom - tex.top) * TextRenderer::m_uTexHeight * 2 / rtSize.cy;

    if (c != 0x0020 && c != 0x00A0) { // Space and No-Break Space
      const float left = drawX;
      const float right = drawX + Width;
      const float top = drawY;
      const float bottom = drawY - Height;
      m_vertices.push_back({ {left,  top,    0.0f}, {tex.left,  tex.top} });
      m_vertices.push_back({ {right, bottom, 0.0f}, {tex.right, tex.bottom} });
      m_vertices.push_back({ {left,  bottom, 0.0f}, {tex.left,  tex.bottom} });
      m_vertices.push_back({ {left,  top,    0.0f}, {tex.left,  tex.top} });
      m_vertices.push_back({ {right, top,    0.0f}, {tex.right, tex.top} });
      m_vertices.push_back({ {right, bottom, 0.0f}, {tex.right, tex.bottom} });
      //nVertices += 6;

      if (m_vertices.size() > (MAX_NUM_VERTS - 6)) {
        m_Context.TransitionResource(TextRenderer::m_pTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);


        m_Context.SetDynamicDescriptor(0, 0, TextRenderer::m_pTexture.GetSRV());
        m_Context.SetDynamicConstantBufferView(1, sizeof(CONSTANT_BUFFER_COLOR), &colorconstant);

        m_Context.SetDynamicVB(0, m_vertices.size(), sizeof(Font12Vertex), m_vertices.data());
        m_Context.Draw(m_vertices.size());
        m_vertices.clear();
       
      }
    }

    drawX += Width;
  }


  m_Context.TransitionResource(TextRenderer::m_pTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
  
  
  m_Context.SetDynamicDescriptor(0, 0, TextRenderer::m_pTexture.GetSRV());
  m_Context.SetDynamicConstantBufferView(1, sizeof(CONSTANT_BUFFER_COLOR), &colorconstant);

  m_Context.SetDynamicVB(0, m_vertices.size(), sizeof(Font12Vertex), m_vertices.data());
  m_Context.Draw(m_vertices.size());
}