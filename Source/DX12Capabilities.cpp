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

#pragma once

#include "stdafx.h"

#include <dxgi1_6.h>
#include "Helper.h"
#include "DX12Capabilities.h"
#include <string>
#include "DX12Engine.h"


namespace D3D12Engine
{
#define ENUMNAME(a) case a: return TEXT(#a)

  const TCHAR* FormatName(DXGI_FORMAT format)
  {
    switch (format)
    {
      ENUMNAME(DXGI_FORMAT_R32G32B32A32_TYPELESS);
      ENUMNAME(DXGI_FORMAT_R32G32B32A32_FLOAT);
      ENUMNAME(DXGI_FORMAT_R32G32B32A32_UINT);
      ENUMNAME(DXGI_FORMAT_R32G32B32A32_SINT);
      ENUMNAME(DXGI_FORMAT_R32G32B32_TYPELESS);
      ENUMNAME(DXGI_FORMAT_R32G32B32_FLOAT);
      ENUMNAME(DXGI_FORMAT_R32G32B32_UINT);
      ENUMNAME(DXGI_FORMAT_R32G32B32_SINT);
      ENUMNAME(DXGI_FORMAT_R16G16B16A16_TYPELESS);
      ENUMNAME(DXGI_FORMAT_R16G16B16A16_FLOAT);
      ENUMNAME(DXGI_FORMAT_R16G16B16A16_UNORM);
      ENUMNAME(DXGI_FORMAT_R16G16B16A16_UINT);
      ENUMNAME(DXGI_FORMAT_R16G16B16A16_SNORM);
      ENUMNAME(DXGI_FORMAT_R16G16B16A16_SINT);
      ENUMNAME(DXGI_FORMAT_R32G32_TYPELESS);
      ENUMNAME(DXGI_FORMAT_R32G32_FLOAT);
      ENUMNAME(DXGI_FORMAT_R32G32_UINT);
      ENUMNAME(DXGI_FORMAT_R32G32_SINT);
      ENUMNAME(DXGI_FORMAT_R32G8X24_TYPELESS);
      ENUMNAME(DXGI_FORMAT_D32_FLOAT_S8X24_UINT);
      ENUMNAME(DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS);
      ENUMNAME(DXGI_FORMAT_X32_TYPELESS_G8X24_UINT);
      ENUMNAME(DXGI_FORMAT_R10G10B10A2_TYPELESS);
      ENUMNAME(DXGI_FORMAT_R10G10B10A2_UNORM);
      ENUMNAME(DXGI_FORMAT_R10G10B10A2_UINT);
      ENUMNAME(DXGI_FORMAT_R11G11B10_FLOAT);
      ENUMNAME(DXGI_FORMAT_R8G8B8A8_TYPELESS);
      ENUMNAME(DXGI_FORMAT_R8G8B8A8_UNORM);
      ENUMNAME(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
      ENUMNAME(DXGI_FORMAT_R8G8B8A8_UINT);
      ENUMNAME(DXGI_FORMAT_R8G8B8A8_SNORM);
      ENUMNAME(DXGI_FORMAT_R8G8B8A8_SINT);
      ENUMNAME(DXGI_FORMAT_R16G16_TYPELESS);
      ENUMNAME(DXGI_FORMAT_R16G16_FLOAT);
      ENUMNAME(DXGI_FORMAT_R16G16_UNORM);
      ENUMNAME(DXGI_FORMAT_R16G16_UINT);
      ENUMNAME(DXGI_FORMAT_R16G16_SNORM);
      ENUMNAME(DXGI_FORMAT_R16G16_SINT);
      ENUMNAME(DXGI_FORMAT_R32_TYPELESS);
      ENUMNAME(DXGI_FORMAT_D32_FLOAT);
      ENUMNAME(DXGI_FORMAT_R32_FLOAT);
      ENUMNAME(DXGI_FORMAT_R32_UINT);
      ENUMNAME(DXGI_FORMAT_R32_SINT);
      ENUMNAME(DXGI_FORMAT_R24G8_TYPELESS);
      ENUMNAME(DXGI_FORMAT_D24_UNORM_S8_UINT);
      ENUMNAME(DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
      ENUMNAME(DXGI_FORMAT_X24_TYPELESS_G8_UINT);
      ENUMNAME(DXGI_FORMAT_R8G8_TYPELESS);
      ENUMNAME(DXGI_FORMAT_R8G8_UNORM);
      ENUMNAME(DXGI_FORMAT_R8G8_UINT);
      ENUMNAME(DXGI_FORMAT_R8G8_SNORM);
      ENUMNAME(DXGI_FORMAT_R8G8_SINT);
      ENUMNAME(DXGI_FORMAT_R16_TYPELESS);
      ENUMNAME(DXGI_FORMAT_R16_FLOAT);
      ENUMNAME(DXGI_FORMAT_D16_UNORM);
      ENUMNAME(DXGI_FORMAT_R16_UNORM);
      ENUMNAME(DXGI_FORMAT_R16_UINT);
      ENUMNAME(DXGI_FORMAT_R16_SNORM);
      ENUMNAME(DXGI_FORMAT_R16_SINT);
      ENUMNAME(DXGI_FORMAT_R8_TYPELESS);
      ENUMNAME(DXGI_FORMAT_R8_UNORM);
      ENUMNAME(DXGI_FORMAT_R8_UINT);
      ENUMNAME(DXGI_FORMAT_R8_SNORM);
      ENUMNAME(DXGI_FORMAT_R8_SINT);
      ENUMNAME(DXGI_FORMAT_A8_UNORM);
      ENUMNAME(DXGI_FORMAT_R1_UNORM);
      ENUMNAME(DXGI_FORMAT_R9G9B9E5_SHAREDEXP);
      ENUMNAME(DXGI_FORMAT_R8G8_B8G8_UNORM);
      ENUMNAME(DXGI_FORMAT_G8R8_G8B8_UNORM);
      ENUMNAME(DXGI_FORMAT_BC1_TYPELESS);
      ENUMNAME(DXGI_FORMAT_BC1_UNORM);
      ENUMNAME(DXGI_FORMAT_BC1_UNORM_SRGB);
      ENUMNAME(DXGI_FORMAT_BC2_TYPELESS);
      ENUMNAME(DXGI_FORMAT_BC2_UNORM);
      ENUMNAME(DXGI_FORMAT_BC2_UNORM_SRGB);
      ENUMNAME(DXGI_FORMAT_BC3_TYPELESS);
      ENUMNAME(DXGI_FORMAT_BC3_UNORM);
      ENUMNAME(DXGI_FORMAT_BC3_UNORM_SRGB);
      ENUMNAME(DXGI_FORMAT_BC4_TYPELESS);
      ENUMNAME(DXGI_FORMAT_BC4_UNORM);
      ENUMNAME(DXGI_FORMAT_BC4_SNORM);
      ENUMNAME(DXGI_FORMAT_BC5_TYPELESS);
      ENUMNAME(DXGI_FORMAT_BC5_UNORM);
      ENUMNAME(DXGI_FORMAT_BC5_SNORM);
      ENUMNAME(DXGI_FORMAT_B5G6R5_UNORM);
      ENUMNAME(DXGI_FORMAT_B5G5R5A1_UNORM);
      ENUMNAME(DXGI_FORMAT_B8G8R8A8_UNORM);
      ENUMNAME(DXGI_FORMAT_B8G8R8X8_UNORM);
      ENUMNAME(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM);
      ENUMNAME(DXGI_FORMAT_B8G8R8A8_TYPELESS);
      ENUMNAME(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
      ENUMNAME(DXGI_FORMAT_B8G8R8X8_TYPELESS);
      ENUMNAME(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB);
      ENUMNAME(DXGI_FORMAT_BC6H_TYPELESS);
      ENUMNAME(DXGI_FORMAT_BC6H_UF16);
      ENUMNAME(DXGI_FORMAT_BC6H_SF16);
      ENUMNAME(DXGI_FORMAT_BC7_TYPELESS);
      ENUMNAME(DXGI_FORMAT_BC7_UNORM);
      ENUMNAME(DXGI_FORMAT_BC7_UNORM_SRGB);
      ENUMNAME(DXGI_FORMAT_AYUV);
      ENUMNAME(DXGI_FORMAT_Y410);
      ENUMNAME(DXGI_FORMAT_Y416);
      ENUMNAME(DXGI_FORMAT_NV12);
      ENUMNAME(DXGI_FORMAT_P010);
      ENUMNAME(DXGI_FORMAT_P016);
      ENUMNAME(DXGI_FORMAT_420_OPAQUE);
      ENUMNAME(DXGI_FORMAT_YUY2);
      ENUMNAME(DXGI_FORMAT_Y210);
      ENUMNAME(DXGI_FORMAT_Y216);
      ENUMNAME(DXGI_FORMAT_NV11);
      ENUMNAME(DXGI_FORMAT_AI44);
      ENUMNAME(DXGI_FORMAT_IA44);
      ENUMNAME(DXGI_FORMAT_P8);
      ENUMNAME(DXGI_FORMAT_A8P8);
      ENUMNAME(DXGI_FORMAT_B4G4R4A4_UNORM);
      ENUMNAME(DXGI_FORMAT_P208);
      ENUMNAME(DXGI_FORMAT_V208);
      ENUMNAME(DXGI_FORMAT_V408);

    default:
      return TEXT("DXGI_FORMAT_UNKNOWN");
    }
  }
  std::wstring FormatCaps(VideoCapabilities caps)
  {
    
    std::wstring str;
    str = L"[Texture2D]: ";
    str.insert(str.size(), caps.Texture2D ? L"Yes" : L"No");
    str.insert(str.size(), L" [Videoprocessor Input]:");
    str.insert(str.size(), caps.Input ? L"Yes" : L"No");
    str.insert(str.size(), L" [Videoprocessor Output]:");
    str.insert(str.size(), caps.Output ? L"Yes" : L"No");
    str.insert(str.size(), L" [Encode]:");
    str.insert(str.size(), caps.Encoder ? L"Yes" : L"No");
    
    //cap += caps.Texture2D ? L"Yes":L"No";
    return str;
  }

  std::map<DXGI_FORMAT, VideoCapabilities> D3D12Engine::GetCapabilities()
  {
    std::map<DXGI_FORMAT, VideoCapabilities> capsv;
    static const DXGI_FORMAT cfsVideo[] =
    {
        DXGI_FORMAT_NV12,
        DXGI_FORMAT_420_OPAQUE,
        DXGI_FORMAT_YUY2,
        DXGI_FORMAT_AYUV,
        DXGI_FORMAT_Y410,
        DXGI_FORMAT_Y416,
        DXGI_FORMAT_P010,
        DXGI_FORMAT_P016,
        DXGI_FORMAT_Y210,
        DXGI_FORMAT_Y216,
        DXGI_FORMAT_NV11,
        DXGI_FORMAT_AI44,
        DXGI_FORMAT_IA44,
        DXGI_FORMAT_P8,
        DXGI_FORMAT_A8P8,
    };
    std::wstring output;
    for (UINT i = 0; i < std::size(cfsVideo); ++i)
    {
      D3D12_FEATURE_DATA_FORMAT_SUPPORT fmtSupport = {
          cfsVideo[i], D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE,
      };
      if (FAILED(D3D12Engine::g_Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,
        &fmtSupport, sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT))))
      {
        fmtSupport.Support1 = D3D12_FORMAT_SUPPORT1_NONE;
        fmtSupport.Support2 = D3D12_FORMAT_SUPPORT2_NONE;
      }

      bool any = (fmtSupport.Support1 & (D3D12_FORMAT_SUPPORT1_TEXTURE2D
        | D3D12_FORMAT_SUPPORT1_VIDEO_PROCESSOR_INPUT
        | D3D12_FORMAT_SUPPORT1_VIDEO_PROCESSOR_OUTPUT
        | D3D12_FORMAT_SUPPORT1_VIDEO_ENCODER)) ? true : false;
      VideoCapabilities caps;
      caps.Texture2D = (fmtSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D);
      caps.Input = (fmtSupport.Support1 & D3D12_FORMAT_SUPPORT1_VIDEO_PROCESSOR_INPUT);
      caps.Output = (fmtSupport.Support1 & D3D12_FORMAT_SUPPORT1_VIDEO_PROCESSOR_OUTPUT);
      caps.Encoder = (fmtSupport.Support1 & D3D12_FORMAT_SUPPORT1_VIDEO_ENCODER);
      capsv.insert({ cfsVideo[i], caps });
      output = FormatName(cfsVideo[i]);
      output = FormatCaps(caps);
      DLog(L"D3D12Device video capabilties for format DXGI adapter: {}", output.c_str());
      
    }
    return capsv;
  }

  void D3D12Engine::FillD3D12Capabilities()
  {
    
  }
};

#if 0
HRESULT D3D_FeatureLevel(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3, PRINTCBINFO* pPrintInfo)
{
  auto fl = static_cast<D3D_FEATURE_LEVEL>(lParam1);
  if (lParam2 == 0)
    return S_OK;

  ID3D10Device* pD3D10 = nullptr;
  ID3D10Device1* pD3D10_1 = nullptr;
  ID3D11Device* pD3D11 = nullptr;
  ID3D11Device1* pD3D11_1 = nullptr;
  ID3D11Device2* pD3D11_2 = nullptr;
  ID3D11Device3* pD3D11_3 = nullptr;
  ID3D12Device* pD3D12 = nullptr;

  auto d3dVer = static_cast<unsigned int>(lParam3 & 0xff);
  auto d3dType = static_cast<D3D_DRIVER_TYPE>((lParam3 & 0xff00) >> 8);

  if (d3dVer == 10)
  {
    pD3D12 = reinterpret_cast<ID3D12Device*>(lParam2);
  }
  else if (d3dVer == 5)
  {
    pD3D11_3 = reinterpret_cast<ID3D11Device3*>(lParam2);
  }
  else
  {
    if (fl > D3D_FEATURE_LEVEL_11_1)
      fl = D3D_FEATURE_LEVEL_11_1;

    if (d3dVer == 4)
    {
      pD3D11_2 = reinterpret_cast<ID3D11Device2*>(lParam2);
    }
    else if (d3dVer == 3)
    {
      pD3D11_1 = reinterpret_cast<ID3D11Device1*>(lParam2);
    }
    else
    {
      if (fl > D3D_FEATURE_LEVEL_11_0)
        fl = D3D_FEATURE_LEVEL_11_0;

      if (d3dVer == 2)
      {
        pD3D11 = reinterpret_cast<ID3D11Device*>(lParam2);
      }
      else
      {
        if (fl > D3D_FEATURE_LEVEL_10_1)
          fl = D3D_FEATURE_LEVEL_10_1;

        if (d3dVer == 1)
        {
          pD3D10_1 = reinterpret_cast<ID3D10Device1*>(lParam2);
        }
        else
        {
          if (fl > D3D_FEATURE_LEVEL_10_0)
            fl = D3D_FEATURE_LEVEL_10_0;

          pD3D10 = reinterpret_cast<ID3D10Device*>(lParam2);
        }
      }
    }
  }

  if (!pPrintInfo)
  {
    LVAddColumn(g_hwndLV, 0, "Name", 30);
    LVAddColumn(g_hwndLV, 1, "Value", 60);
  }

  const char* shaderModel = nullptr;
  const char* computeShader = c_szNo;
  const char* maxTexDim = nullptr;
  const char* maxCubeDim = nullptr;
  const char* maxVolDim = nullptr;
  const char* maxTexRepeat = nullptr;
  const char* maxAnisotropy = nullptr;
  const char* maxPrimCount = "4294967296";
  const char* maxInputSlots = nullptr;
  const char* mrt = nullptr;
  const char* extFormats = nullptr;
  const char* x2_10BitFormat = nullptr;
  const char* logic_ops = c_szNo;
  const char* cb_partial = c_szNA;
  const char* cb_offsetting = c_szNA;
  const char* uavSlots = nullptr;
  const char* uavEveryStage = nullptr;
  const char* uavOnlyRender = nullptr;
  const char* nonpow2 = nullptr;
  const char* bpp16 = c_szNo;
  const char* shadows = nullptr;
  const char* cubeRT = nullptr;
  const char* tiled_rsc = nullptr;
  const char* binding_rsc = nullptr;
  const char* minmaxfilter = nullptr;
  const char* mapdefaultbuff = nullptr;
  const char* consrv_rast = nullptr;
  const char* rast_ordered_views = nullptr;
  const char* ps_stencil_ref = nullptr;
  const char* instancing = nullptr;
  const char* vrs = nullptr;
  const char* meshShaders = nullptr;
  const char* dxr = nullptr;

  BOOL _10level9 = FALSE;

  switch (fl)
  {
  case D3D_FEATURE_LEVEL_12_2:
    if (pD3D12)
    {
      switch (GetD3D12ShaderModel(pD3D12))
      {
      case D3D_SHADER_MODEL_6_7:
        shaderModel = "6.7 (Optional)";
        computeShader = "Yes (CS 6.7)";
        break;
      case D3D_SHADER_MODEL_6_6:
        shaderModel = "6.6 (Optional)";
        computeShader = "Yes (CS 6.6)";
        break;
      default:
        shaderModel = "6.5";
        computeShader = "Yes (CS 6.5)";
        break;
      }
      vrs = "Yes - Tier 2";
      meshShaders = c_szYes;
      dxr = "Yes - Tier 1.1";
    }
    // Fall-through

  case D3D_FEATURE_LEVEL_12_1:
  case D3D_FEATURE_LEVEL_12_0:
    if (!shaderModel)
    {
      switch (GetD3D12ShaderModel(pD3D12))
      {
      case D3D_SHADER_MODEL_6_7:
        shaderModel = "6.7 (Optional)";
        computeShader = "Yes (CS 6.7)";
        break;
      case D3D_SHADER_MODEL_6_6:
        shaderModel = "6.6 (Optional)";
        computeShader = "Yes (CS 6.6)";
        break;
      case D3D_SHADER_MODEL_6_5:
        shaderModel = "6.5 (Optional)";
        computeShader = "Yes (CS 6.5)";
        break;
      case D3D_SHADER_MODEL_6_4:
        shaderModel = "6.4 (Optional)";
        computeShader = "Yes (CS 6.4)";
        break;
      case D3D_SHADER_MODEL_6_3:
        shaderModel = "6.3 (Optional)";
        computeShader = "Yes (CS 6.3)";
        break;
      case D3D_SHADER_MODEL_6_2:
        shaderModel = "6.2 (Optional)";
        computeShader = "Yes (CS 6.2)";
        break;
      case D3D_SHADER_MODEL_6_1:
        shaderModel = "6.1 (Optional)";
        computeShader = "Yes (CS 6.1)";
        break;
      case D3D_SHADER_MODEL_6_0:
        shaderModel = "6.0 (Optional)";
        computeShader = "Yes (CS 6.0)";
        break;
      default:
        shaderModel = "5.1";
        computeShader = "Yes (CS 5.1)";
        break;
      }
    }
    extFormats = c_szYes;
    x2_10BitFormat = c_szYes;
    logic_ops = c_szYes;
    cb_partial = c_szYes;
    cb_offsetting = c_szYes;
    uavEveryStage = c_szYes;
    uavOnlyRender = "16";
    nonpow2 = "Full";
    bpp16 = c_szYes;
    instancing = c_szYes;

    if (pD3D12)
    {
      maxTexDim = XTOSTRING(D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
      maxCubeDim = XTOSTRING(D3D12_REQ_TEXTURECUBE_DIMENSION);
      maxVolDim = XTOSTRING(D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
      maxTexRepeat = XTOSTRING(D3D12_REQ_FILTERING_HW_ADDRESSABLE_RESOURCE_DIMENSION);
      maxAnisotropy = XTOSTRING(D3D12_REQ_MAXANISOTROPY);
      maxInputSlots = XTOSTRING(D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
      mrt = XTOSTRING(D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);
      uavSlots = XTOSTRING(D3D12_UAV_SLOT_COUNT);

      D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12opts = {};
      HRESULT hr = pD3D12->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12opts, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
      if (FAILED(hr))
        memset(&d3d12opts, 0, sizeof(d3d12opts));

      switch (d3d12opts.TiledResourcesTier)
      {
        // 12.0 & 12.1 should be T2 or greater, 12.2 is T3 or greater
      case D3D12_TILED_RESOURCES_TIER_2:  tiled_rsc = "Yes - Tier 2"; break;
      case D3D12_TILED_RESOURCES_TIER_3:  tiled_rsc = "Yes - Tier 3"; break;
      case D3D12_TILED_RESOURCES_TIER_4:  tiled_rsc = "Yes - Tier 4"; break;
      default:                            tiled_rsc = c_szYes;        break;
      }

      switch (d3d12opts.ResourceBindingTier)
      {
        // 12.0 & 12.1 should be T2 or greater, 12.2 is T3 or greater
      case D3D12_RESOURCE_BINDING_TIER_2: binding_rsc = "Yes - Tier 2"; break;
      case D3D12_RESOURCE_BINDING_TIER_3: binding_rsc = "Yes - Tier 3"; break;
      default:                            binding_rsc = c_szYes;        break;
      }

      if (fl >= D3D_FEATURE_LEVEL_12_1)
      {
        switch (d3d12opts.ConservativeRasterizationTier)
        {
          // 12.1 requires T1, 12.2 requires T3
        case D3D12_CONSERVATIVE_RASTERIZATION_TIER_1:   consrv_rast = "Yes - Tier 1";  break;
        case D3D12_CONSERVATIVE_RASTERIZATION_TIER_2:   consrv_rast = "Yes - Tier 2";  break;
        case D3D12_CONSERVATIVE_RASTERIZATION_TIER_3:   consrv_rast = "Yes - Tier 3";  break;
        default:                                        consrv_rast = c_szYes;         break;
        }

        rast_ordered_views = c_szYes;
      }
      else
      {
        switch (d3d12opts.ConservativeRasterizationTier)
        {
        case D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED:   consrv_rast = c_szOptNo; break;
        case D3D12_CONSERVATIVE_RASTERIZATION_TIER_1:               consrv_rast = "Optional (Yes - Tier 1)";  break;
        case D3D12_CONSERVATIVE_RASTERIZATION_TIER_2:               consrv_rast = "Optional (Yes - Tier 2)";  break;
        case D3D12_CONSERVATIVE_RASTERIZATION_TIER_3:               consrv_rast = "Optional (Yes - Tier 3)";  break;
        default:                                                    consrv_rast = c_szOptYes; break;
        }

        rast_ordered_views = (d3d12opts.ROVsSupported) ? c_szOptYes : c_szOptNo;
      }

      ps_stencil_ref = (d3d12opts.PSSpecifiedStencilRefSupported) ? c_szOptYes : c_szOptNo;
      minmaxfilter = c_szYes;
      mapdefaultbuff = c_szYes;
    }
    else if (pD3D11_3)
    {
      maxTexDim = XTOSTRING(D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION);
      maxCubeDim = XTOSTRING(D3D11_REQ_TEXTURECUBE_DIMENSION);
      maxVolDim = XTOSTRING(D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
      maxTexRepeat = XTOSTRING(D3D11_REQ_FILTERING_HW_ADDRESSABLE_RESOURCE_DIMENSION);
      maxAnisotropy = XTOSTRING(D3D11_REQ_MAXANISOTROPY);
      maxInputSlots = XTOSTRING(D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
      mrt = XTOSTRING(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);
      uavSlots = XTOSTRING(D3D11_1_UAV_SLOT_COUNT);

      D3D11_TILED_RESOURCES_TIER tiled = D3D11_TILED_RESOURCES_NOT_SUPPORTED;
      bool bMinMaxFilter, bMapDefaultBuff;
      CheckD3D11Ops1(pD3D11_3, tiled, bMinMaxFilter, bMapDefaultBuff);

      D3D11_CONSERVATIVE_RASTERIZATION_TIER crast = D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED;
      bool rovs, pssref;
      CheckD3D11Ops2(pD3D11_3, tiled, crast, rovs, pssref);

      switch (tiled)
      {
        // 12.0 & 12.1 should be T2 or greater, 12.2 is T3 or greater
      case D3D11_TILED_RESOURCES_TIER_2:          tiled_rsc = "Yes - Tier 2";  break;
      case D3D11_TILED_RESOURCES_TIER_3:          tiled_rsc = "Yes - Tier 3";  break;
      default:                                    tiled_rsc = c_szYes;         break;
      }

      if (fl >= D3D_FEATURE_LEVEL_12_1)
      {
        switch (crast)
        {
          // 12.1 requires T1
        case D3D11_CONSERVATIVE_RASTERIZATION_TIER_1:           consrv_rast = "Yes - Tier 1";  break;
        case D3D11_CONSERVATIVE_RASTERIZATION_TIER_2:           consrv_rast = "Yes - Tier 2";  break;
        case D3D11_CONSERVATIVE_RASTERIZATION_TIER_3:           consrv_rast = "Yes - Tier 3";  break;
        default:                                                consrv_rast = c_szYes;         break;
        }

        rast_ordered_views = c_szYes;
      }
      else
      {
        switch (crast)
        {
        case D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED:    consrv_rast = c_szOptNo; break;
        case D3D11_CONSERVATIVE_RASTERIZATION_TIER_1:           consrv_rast = "Optional (Yes - Tier 1)";  break;
        case D3D11_CONSERVATIVE_RASTERIZATION_TIER_2:           consrv_rast = "Optional (Yes - Tier 2)";  break;
        case D3D11_CONSERVATIVE_RASTERIZATION_TIER_3:           consrv_rast = "Optional (Yes - Tier 3)";  break;
        default:                                                consrv_rast = c_szOptYes; break;
        }

        rast_ordered_views = (rovs) ? c_szOptYes : c_szOptNo;
      }

      ps_stencil_ref = (pssref) ? c_szOptYes : c_szOptNo;
      minmaxfilter = c_szYes;
      mapdefaultbuff = c_szYes;
    }
    break;

  case D3D_FEATURE_LEVEL_11_1:
    shaderModel = "5.0";
    computeShader = "Yes (CS 5.0)";
    maxTexDim = XTOSTRING(D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    maxCubeDim = XTOSTRING(D3D11_REQ_TEXTURECUBE_DIMENSION);
    maxVolDim = XTOSTRING(D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
    maxTexRepeat = XTOSTRING(D3D11_REQ_FILTERING_HW_ADDRESSABLE_RESOURCE_DIMENSION);
    maxAnisotropy = XTOSTRING(D3D11_REQ_MAXANISOTROPY);
    maxInputSlots = XTOSTRING(D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
    mrt = XTOSTRING(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);
    extFormats = c_szYes;
    x2_10BitFormat = c_szYes;
    logic_ops = c_szYes;
    cb_partial = c_szYes;
    cb_offsetting = c_szYes;
    uavSlots = XTOSTRING(D3D11_1_UAV_SLOT_COUNT);
    uavEveryStage = c_szYes;
    uavOnlyRender = "16";
    nonpow2 = "Full";
    bpp16 = c_szYes;
    instancing = c_szYes;

    if (pD3D12)
    {
      shaderModel = "5.1";
      computeShader = "Yes (CS 5.1)";

      D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12opts = {};
      HRESULT hr = pD3D12->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12opts, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
      if (FAILED(hr))
        memset(&d3d12opts, 0, sizeof(d3d12opts));

      switch (d3d12opts.TiledResourcesTier)
      {
      case D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED:  tiled_rsc = c_szOptNo; break;
      case D3D12_TILED_RESOURCES_TIER_1:              tiled_rsc = "Optional (Yes - Tier 1)";  break;
      case D3D12_TILED_RESOURCES_TIER_2:              tiled_rsc = "Optional (Yes - Tier 2)";  break;
      case D3D12_TILED_RESOURCES_TIER_3:              tiled_rsc = "Optional (Yes - Tier 3)";  break;
      case D3D12_TILED_RESOURCES_TIER_4:              tiled_rsc = "Optional (Yes - Tier 4)";  break;
      default:                                        tiled_rsc = c_szOptYes; break;
      }

      switch (d3d12opts.ResourceBindingTier)
      {
      case D3D12_RESOURCE_BINDING_TIER_1: binding_rsc = "Yes - Tier 1"; break;
      case D3D12_RESOURCE_BINDING_TIER_2: binding_rsc = "Yes - Tier 2"; break;
      case D3D12_RESOURCE_BINDING_TIER_3: binding_rsc = "Yes - Tier 3"; break;
      default:                            binding_rsc = c_szYes;        break;
      }

      switch (d3d12opts.ConservativeRasterizationTier)
      {
      case D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED:   consrv_rast = c_szOptNo; break;
      case D3D12_CONSERVATIVE_RASTERIZATION_TIER_1:               consrv_rast = "Optional (Yes - Tier 1)";  break;
      case D3D12_CONSERVATIVE_RASTERIZATION_TIER_2:               consrv_rast = "Optional (Yes - Tier 2)";  break;
      case D3D12_CONSERVATIVE_RASTERIZATION_TIER_3:               consrv_rast = "Optional (Yes - Tier 3)";  break;
      default:                                                    consrv_rast = c_szOptYes; break;
      }

      rast_ordered_views = (d3d12opts.ROVsSupported) ? c_szOptYes : c_szOptNo;
      ps_stencil_ref = (d3d12opts.PSSpecifiedStencilRefSupported) ? c_szOptYes : c_szOptNo;
      minmaxfilter = c_szYes;
      mapdefaultbuff = c_szYes;
    }
    else if (pD3D11_3)
    {
      D3D11_TILED_RESOURCES_TIER tiled = D3D11_TILED_RESOURCES_NOT_SUPPORTED;
      bool bMinMaxFilter, bMapDefaultBuff;
      CheckD3D11Ops1(pD3D11_3, tiled, bMinMaxFilter, bMapDefaultBuff);

      D3D11_CONSERVATIVE_RASTERIZATION_TIER crast = D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED;
      bool rovs, pssref;
      CheckD3D11Ops2(pD3D11_3, tiled, crast, rovs, pssref);

      switch (tiled)
      {
      case D3D11_TILED_RESOURCES_NOT_SUPPORTED:   tiled_rsc = c_szOptNo; break;
      case D3D11_TILED_RESOURCES_TIER_1:          tiled_rsc = "Optional (Yes - Tier 1)";  break;
      case D3D11_TILED_RESOURCES_TIER_2:          tiled_rsc = "Optional (Yes - Tier 2)";  break;
      case D3D11_TILED_RESOURCES_TIER_3:          tiled_rsc = "Optional (Yes - Tier 3)";  break;
      default:                                    tiled_rsc = c_szOptYes; break;
      }

      switch (crast)
      {
      case D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED:    consrv_rast = c_szOptNo; break;
      case D3D11_CONSERVATIVE_RASTERIZATION_TIER_1:           consrv_rast = "Optional (Yes - Tier 1)";  break;
      case D3D11_CONSERVATIVE_RASTERIZATION_TIER_2:           consrv_rast = "Optional (Yes - Tier 2)";  break;
      case D3D11_CONSERVATIVE_RASTERIZATION_TIER_3:           consrv_rast = "Optional (Yes - Tier 3)";  break;
      default:                                                consrv_rast = c_szOptYes; break;
      }

      rast_ordered_views = (rovs) ? c_szOptYes : c_szOptNo;
      ps_stencil_ref = (pssref) ? c_szOptYes : c_szOptNo;
      minmaxfilter = bMinMaxFilter ? c_szOptYes : c_szOptNo;
      mapdefaultbuff = bMapDefaultBuff ? c_szOptYes : c_szOptNo;
    }
    else if (pD3D11_2)
    {
      D3D11_TILED_RESOURCES_TIER tiled = D3D11_TILED_RESOURCES_NOT_SUPPORTED;
      bool bMinMaxFilter, bMapDefaultBuff;
      CheckD3D11Ops1(pD3D11_2, tiled, bMinMaxFilter, bMapDefaultBuff);

      switch (tiled)
      {
      case D3D11_TILED_RESOURCES_NOT_SUPPORTED:   tiled_rsc = c_szOptNo; break;
      case D3D11_TILED_RESOURCES_TIER_1:          tiled_rsc = "Optional (Yes - Tier 1)";  break;
      case D3D11_TILED_RESOURCES_TIER_2:          tiled_rsc = "Optional (Yes - Tier 2)";  break;
      default:                                    tiled_rsc = c_szOptYes; break;
      }

      minmaxfilter = bMinMaxFilter ? c_szOptYes : c_szOptNo;
      mapdefaultbuff = bMapDefaultBuff ? c_szOptYes : c_szOptNo;
    }
    break;

  case D3D_FEATURE_LEVEL_11_0:
    shaderModel = "5.0";
    computeShader = "Yes (CS 5.0)";
    maxTexDim = XTOSTRING(D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    maxCubeDim = XTOSTRING(D3D11_REQ_TEXTURECUBE_DIMENSION);
    maxVolDim = XTOSTRING(D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
    maxTexRepeat = XTOSTRING(D3D11_REQ_FILTERING_HW_ADDRESSABLE_RESOURCE_DIMENSION);
    maxAnisotropy = XTOSTRING(D3D11_REQ_MAXANISOTROPY);
    maxInputSlots = XTOSTRING(D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
    mrt = XTOSTRING(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);
    extFormats = c_szYes;
    x2_10BitFormat = c_szYes;
    instancing = c_szYes;

    if (pD3D12)
    {
      shaderModel = "5.1";
      computeShader = "Yes (CS 5.1)";

      D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12opts = {};
      HRESULT hr = pD3D12->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12opts, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
      if (FAILED(hr))
        memset(&d3d12opts, 0, sizeof(d3d12opts));

      switch (d3d12opts.TiledResourcesTier)
      {
      case D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED:  tiled_rsc = c_szOptNo; break;
      case D3D12_TILED_RESOURCES_TIER_1:              tiled_rsc = "Optional (Yes - Tier 1)";  break;
      case D3D12_TILED_RESOURCES_TIER_2:              tiled_rsc = "Optional (Yes - Tier 2)";  break;
      case D3D12_TILED_RESOURCES_TIER_3:              tiled_rsc = "Optional (Yes - Tier 3)";  break;
      case D3D12_TILED_RESOURCES_TIER_4:              tiled_rsc = "Optional (Yes - Tier 4)";  break;
      default:                                        tiled_rsc = c_szOptYes; break;
      }

      switch (d3d12opts.ResourceBindingTier)
      {
      case D3D12_RESOURCE_BINDING_TIER_1: binding_rsc = "Yes - Tier 1"; break;
      case D3D12_RESOURCE_BINDING_TIER_2: binding_rsc = "Yes - Tier 2"; break;
      case D3D12_RESOURCE_BINDING_TIER_3: binding_rsc = "Yes - Tier 3"; break;
      default:                            binding_rsc = c_szYes;        break;
      }

      logic_ops = d3d12opts.OutputMergerLogicOp ? c_szOptYes : c_szOptNo;
      consrv_rast = rast_ordered_views = ps_stencil_ref = minmaxfilter = c_szNo;
      mapdefaultbuff = c_szYes;

      uavSlots = "8";
      uavEveryStage = c_szNo;
      uavOnlyRender = "8";
      nonpow2 = "Full";
    }
    else if (pD3D11_3)
    {
      D3D11_TILED_RESOURCES_TIER tiled = D3D11_TILED_RESOURCES_NOT_SUPPORTED;
      bool bMinMaxFilter, bMapDefaultBuff;
      CheckD3D11Ops1(pD3D11_3, tiled, bMinMaxFilter, bMapDefaultBuff);

      D3D11_FEATURE_DATA_D3D11_OPTIONS2 d3d11opts2 = {};
      HRESULT hr = pD3D11_3->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &d3d11opts2, sizeof(d3d11opts2));
      if (SUCCEEDED(hr))
      {
        // D3D11_FEATURE_DATA_D3D11_OPTIONS1 caps this at Tier 2
        tiled = d3d11opts2.TiledResourcesTier;
      }

      switch (tiled)
      {
      case D3D11_TILED_RESOURCES_NOT_SUPPORTED:   tiled_rsc = c_szOptNo; break;
      case D3D11_TILED_RESOURCES_TIER_1:          tiled_rsc = "Optional (Yes - Tier 1)";  break;
      case D3D11_TILED_RESOURCES_TIER_2:          tiled_rsc = "Optional (Yes - Tier 2)";  break;
      case D3D11_TILED_RESOURCES_TIER_3:          tiled_rsc = "Optional (Yes - Tier 3)";  break;
      default:                                    tiled_rsc = c_szOptYes; break;
      }

      consrv_rast = rast_ordered_views = ps_stencil_ref = minmaxfilter = c_szNo;
      mapdefaultbuff = bMapDefaultBuff ? c_szOptYes : c_szOptNo;
    }
    else if (pD3D11_2)
    {
      D3D11_TILED_RESOURCES_TIER tiled = D3D11_TILED_RESOURCES_NOT_SUPPORTED;
      bool bMinMaxFilter, bMapDefaultBuff;
      CheckD3D11Ops1(pD3D11_2, tiled, bMinMaxFilter, bMapDefaultBuff);

      switch (tiled)
      {
      case D3D11_TILED_RESOURCES_NOT_SUPPORTED:   tiled_rsc = c_szOptNo; break;
      case D3D11_TILED_RESOURCES_TIER_1:          tiled_rsc = "Optional (Yes - Tier 1)";  break;
      case D3D11_TILED_RESOURCES_TIER_2:          tiled_rsc = "Optional (Yes - Tier 2)";  break;
      default:                                    tiled_rsc = c_szOptYes; break;
      }

      minmaxfilter = c_szNo;
      mapdefaultbuff = bMapDefaultBuff ? c_szOptYes : c_szOptNo;
    }

    if (pD3D11_1 || pD3D11_2 || pD3D11_3)
    {
      ID3D11Device* pD3D = (pD3D11_3) ? pD3D11_3 : ((pD3D11_2) ? pD3D11_2 : pD3D11_1);

      bool bLogicOps, bCBpartial, bCBoffsetting;
      CheckD3D11Ops(pD3D, bLogicOps, bCBpartial, bCBoffsetting);
      logic_ops = bLogicOps ? c_szOptYes : c_szOptNo;
      cb_partial = bCBpartial ? c_szOptYes : c_szOptNo;
      cb_offsetting = bCBoffsetting ? c_szOptYes : c_szOptNo;

      BOOL ext, x2, bpp565;
      CheckExtendedFormats(pD3D, ext, x2, bpp565);
      bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;

      uavSlots = "8";
      uavEveryStage = c_szNo;
      uavOnlyRender = "8";
      nonpow2 = "Full";
    }
    break;

  case D3D_FEATURE_LEVEL_10_1:
    shaderModel = "4.x";
    instancing = c_szYes;

    if (pD3D11_3)
    {
      consrv_rast = rast_ordered_views = ps_stencil_ref = c_szNo;
    }

    if (pD3D11_2 || pD3D11_3)
    {
      tiled_rsc = minmaxfilter = mapdefaultbuff = c_szNo;
    }

    if (pD3D11_1 || pD3D11_2 || pD3D11_3)
    {
      ID3D11Device* pD3D = (pD3D11_3) ? pD3D11_3 : ((pD3D11_2) ? pD3D11_2 : pD3D11_1);

      D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS d3d10xhw = {};
      HRESULT hr = pD3D->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &d3d10xhw, sizeof(d3d10xhw));
      if (FAILED(hr))
        memset(&d3d10xhw, 0, sizeof(d3d10xhw));

      if (d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x)
      {
        computeShader = "Optional (Yes - CS 4.x)";
        uavSlots = "1";
        uavEveryStage = c_szNo;
        uavOnlyRender = c_szNo;
      }
      else
      {
        computeShader = c_szOptNo;
      }

      bool bLogicOps, bCBpartial, bCBoffsetting;
      CheckD3D11Ops(pD3D, bLogicOps, bCBpartial, bCBoffsetting);
      logic_ops = bLogicOps ? c_szOptYes : c_szOptNo;
      cb_partial = bCBpartial ? c_szOptYes : c_szOptNo;
      cb_offsetting = bCBoffsetting ? c_szOptYes : c_szOptNo;

      BOOL ext, x2, bpp565;
      CheckExtendedFormats(pD3D, ext, x2, bpp565);

      extFormats = (ext) ? c_szOptYes : c_szOptNo;
      x2_10BitFormat = (x2) ? c_szOptYes : c_szOptNo;
      nonpow2 = "Full";
      bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;
    }
    else if (pD3D11)
    {
      D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS d3d10xhw = {};
      HRESULT hr = pD3D11->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &d3d10xhw, sizeof(d3d10xhw));
      if (FAILED(hr))
        memset(&d3d10xhw, 0, sizeof(d3d10xhw));

      computeShader = (d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x) ? "Optional (Yes - CS 4.x)" : c_szOptNo;

      BOOL ext, x2, bpp565;
      CheckExtendedFormats(pD3D11, ext, x2, bpp565);

      extFormats = (ext) ? c_szOptYes : c_szOptNo;
      x2_10BitFormat = (x2) ? c_szOptYes : c_szOptNo;
    }
    else if (pD3D10_1)
    {
      BOOL ext, x2;
      CheckExtendedFormats(pD3D10_1, ext, x2);

      extFormats = (ext) ? c_szOptYes : c_szOptNo;
      x2_10BitFormat = (x2) ? c_szOptYes : c_szOptNo;
    }
    else if (pD3D10)
    {
      BOOL ext, x2;
      CheckExtendedFormats(pD3D10, ext, x2);

      extFormats = (ext) ? c_szOptYes : c_szOptNo;
      x2_10BitFormat = (x2) ? c_szOptYes : c_szOptNo;
    }

    maxTexDim = XTOSTRING(D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    maxCubeDim = XTOSTRING(D3D10_REQ_TEXTURECUBE_DIMENSION);
    maxVolDim = XTOSTRING(D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
    maxTexRepeat = XTOSTRING(D3D10_REQ_FILTERING_HW_ADDRESSABLE_RESOURCE_DIMENSION);
    maxAnisotropy = XTOSTRING(D3D10_REQ_MAXANISOTROPY);
    maxInputSlots = XTOSTRING(D3D10_1_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
    mrt = XTOSTRING(D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT);
    break;

  case D3D_FEATURE_LEVEL_10_0:
    shaderModel = "4.0";
    instancing = c_szYes;

    if (pD3D11_3)
    {
      consrv_rast = rast_ordered_views = ps_stencil_ref = c_szNo;
    }

    if (pD3D11_2 || pD3D11_3)
    {
      tiled_rsc = minmaxfilter = mapdefaultbuff = c_szNo;
    }

    if (pD3D11_1 || pD3D11_2 || pD3D11_3)
    {
      ID3D11Device* pD3D = (pD3D11_3) ? pD3D11_3 : ((pD3D11_2) ? pD3D11_2 : pD3D11_1);

      D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS d3d10xhw = {};
      HRESULT hr = pD3D->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &d3d10xhw, sizeof(d3d10xhw));
      if (FAILED(hr))
        memset(&d3d10xhw, 0, sizeof(d3d10xhw));

      if (d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x)
      {
        computeShader = "Optional (Yes - CS 4.x)";
        uavSlots = "1";
        uavEveryStage = c_szNo;
        uavOnlyRender = c_szNo;
      }
      else
      {
        computeShader = c_szOptNo;
      }

      bool bLogicOps, bCBpartial, bCBoffsetting;
      CheckD3D11Ops(pD3D, bLogicOps, bCBpartial, bCBoffsetting);
      logic_ops = bLogicOps ? c_szOptYes : c_szOptNo;
      cb_partial = bCBpartial ? c_szOptYes : c_szOptNo;
      cb_offsetting = bCBoffsetting ? c_szOptYes : c_szOptNo;

      BOOL ext, x2, bpp565;
      CheckExtendedFormats(pD3D, ext, x2, bpp565);

      extFormats = (ext) ? c_szOptYes : c_szOptNo;
      x2_10BitFormat = (x2) ? c_szOptYes : c_szOptNo;
      nonpow2 = "Full";
      bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;
    }
    else if (pD3D11)
    {
      D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS d3d10xhw = {};
      HRESULT hr = pD3D11->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &d3d10xhw, sizeof(d3d10xhw));
      if (FAILED(hr))
        memset(&d3d10xhw, 0, sizeof(d3d10xhw));

      computeShader = (d3d10xhw.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x) ? "Optional (Yes - CS 4.0)" : c_szOptNo;

      BOOL ext, x2, bpp565;
      CheckExtendedFormats(pD3D11, ext, x2, bpp565);

      extFormats = (ext) ? c_szOptYes : c_szOptNo;
      x2_10BitFormat = (x2) ? c_szOptYes : c_szOptNo;
    }
    else if (pD3D10_1)
    {
      BOOL ext, x2;
      CheckExtendedFormats(pD3D10_1, ext, x2);

      extFormats = (ext) ? c_szOptYes : c_szOptNo;
      x2_10BitFormat = (x2) ? c_szOptYes : c_szOptNo;
    }
    else if (pD3D10)
    {
      BOOL ext, x2;
      CheckExtendedFormats(pD3D10, ext, x2);

      extFormats = (ext) ? c_szOptYes : c_szOptNo;
      x2_10BitFormat = (x2) ? c_szOptYes : c_szOptNo;
    }

    maxTexDim = XTOSTRING(D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    maxCubeDim = XTOSTRING(D3D10_REQ_TEXTURECUBE_DIMENSION);
    maxVolDim = XTOSTRING(D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
    maxTexRepeat = XTOSTRING(D3D10_REQ_FILTERING_HW_ADDRESSABLE_RESOURCE_DIMENSION);
    maxAnisotropy = XTOSTRING(D3D10_REQ_MAXANISOTROPY);
    maxInputSlots = XTOSTRING(D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
    mrt = XTOSTRING(D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT);
    break;

  case D3D_FEATURE_LEVEL_9_3:
    _10level9 = TRUE;
    shaderModel = "2.0 (4_0_level_9_3) [vs_2_a/ps_2_b]";
    computeShader = c_szNA;
    maxTexDim = XTOSTRING2(D3D_FL9_3_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    maxCubeDim = XTOSTRING2(D3D_FL9_3_REQ_TEXTURECUBE_DIMENSION);
    maxVolDim = XTOSTRING2(D3D_FL9_1_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
    maxTexRepeat = XTOSTRING2(D3D_FL9_3_MAX_TEXTURE_REPEAT);
    maxAnisotropy = XTOSTRING(D3D11_REQ_MAXANISOTROPY);
    maxPrimCount = XTOSTRING2(D3D_FL9_2_IA_PRIMITIVE_MAX_COUNT);
    maxInputSlots = XTOSTRING(D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
    mrt = XTOSTRING2(D3D_FL9_3_SIMULTANEOUS_RENDER_TARGET_COUNT);
    extFormats = c_szYes;
    instancing = c_szYes;

    if (pD3D11_2 || pD3D11_3)
    {
      ID3D11Device* pD3D = (pD3D11_3) ? pD3D11_3 : pD3D11_2;

      cb_partial = c_szYes;
      cb_offsetting = c_szYes;

      BOOL ext, x2, bpp565;
      CheckExtendedFormats(pD3D, ext, x2, bpp565);

      bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;

      bool bNonPow2, bShadows, bInstancing, bCubeRT;
      CheckD3D9Ops1(pD3D, bNonPow2, bShadows, bInstancing, bCubeRT);
      nonpow2 = bNonPow2 ? "Optional (Full)" : "Conditional";
      shadows = bShadows ? c_szOptYes : c_szOptNo;
      cubeRT = bCubeRT ? c_szOptYes : c_szOptNo;
    }
    else if (pD3D11_1)
    {
      cb_partial = c_szYes;
      cb_offsetting = c_szYes;

      BOOL ext, x2, bpp565;
      CheckExtendedFormats(pD3D11_1, ext, x2, bpp565);

      bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;

      bool bNonPow2, bShadows;
      CheckD3D9Ops(pD3D11_1, bNonPow2, bShadows);
      nonpow2 = bNonPow2 ? "Optional (Full)" : "Conditional";
      shadows = bShadows ? c_szOptYes : c_szOptNo;
    }
    break;

  case D3D_FEATURE_LEVEL_9_2:
    _10level9 = TRUE;
    shaderModel = "2.0 (4_0_level_9_1)";
    computeShader = c_szNA;
    maxTexDim = XTOSTRING2(D3D_FL9_1_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    maxCubeDim = XTOSTRING2(D3D_FL9_1_REQ_TEXTURECUBE_DIMENSION);
    maxVolDim = XTOSTRING2(D3D_FL9_1_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
    maxTexRepeat = XTOSTRING2(D3D_FL9_2_MAX_TEXTURE_REPEAT);
    maxAnisotropy = XTOSTRING(D3D11_REQ_MAXANISOTROPY);
    maxPrimCount = XTOSTRING2(D3D_FL9_2_IA_PRIMITIVE_MAX_COUNT);
    maxInputSlots = XTOSTRING(D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
    mrt = XTOSTRING2(D3D_FL9_1_SIMULTANEOUS_RENDER_TARGET_COUNT);
    extFormats = c_szYes;
    instancing = c_szNo;

    if (pD3D11_2 || pD3D11_3)
    {
      ID3D11Device* pD3D = (pD3D11_3) ? pD3D11_3 : pD3D11_2;

      cb_partial = c_szYes;
      cb_offsetting = c_szYes;

      BOOL ext, x2, bpp565;
      CheckExtendedFormats(pD3D, ext, x2, bpp565);

      bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;

      bool bNonPow2, bShadows, bInstancing, bCubeRT;
      CheckD3D9Ops1(pD3D, bNonPow2, bShadows, bInstancing, bCubeRT);
      nonpow2 = bNonPow2 ? "Optional (Full)" : "Conditional";
      shadows = bShadows ? c_szOptYes : c_szOptNo;
      instancing = bInstancing ? "Optional (Simple)" : c_szOptNo;
      cubeRT = bCubeRT ? c_szOptYes : c_szOptNo;
    }
    else if (pD3D11_1)
    {
      cb_partial = c_szYes;
      cb_offsetting = c_szYes;

      BOOL ext, x2, bpp565;
      CheckExtendedFormats(pD3D11_1, ext, x2, bpp565);

      bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;

      bool bNonPow2, bShadows;
      CheckD3D9Ops(pD3D11_1, bNonPow2, bShadows);
      nonpow2 = bNonPow2 ? "Optional (Full)" : "Conditional";
      shadows = bShadows ? c_szOptYes : c_szOptNo;
    }
    break;

  case D3D_FEATURE_LEVEL_9_1:
    _10level9 = TRUE;
    shaderModel = "2.0 (4_0_level_9_1)";
    computeShader = c_szNA;
    maxTexDim = XTOSTRING2(D3D_FL9_1_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    maxCubeDim = XTOSTRING2(D3D_FL9_1_REQ_TEXTURECUBE_DIMENSION);
    maxVolDim = XTOSTRING2(D3D_FL9_1_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
    maxTexRepeat = XTOSTRING2(D3D_FL9_1_MAX_TEXTURE_REPEAT);
    maxAnisotropy = XTOSTRING2(D3D_FL9_1_DEFAULT_MAX_ANISOTROPY);
    maxPrimCount = XTOSTRING2(D3D_FL9_1_IA_PRIMITIVE_MAX_COUNT);
    maxInputSlots = XTOSTRING(D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
    mrt = XTOSTRING2(D3D_FL9_1_SIMULTANEOUS_RENDER_TARGET_COUNT);
    extFormats = c_szYes;
    instancing = c_szNo;

    if (pD3D11_2 || pD3D11_3)
    {
      ID3D11Device* pD3D = (pD3D11_3) ? pD3D11_3 : pD3D11_2;

      cb_partial = c_szYes;
      cb_offsetting = c_szYes;

      BOOL ext, x2, bpp565;
      CheckExtendedFormats(pD3D, ext, x2, bpp565);

      bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;

      bool bNonPow2, bShadows, bInstancing, bCubeRT;
      CheckD3D9Ops1(pD3D, bNonPow2, bShadows, bInstancing, bCubeRT);
      nonpow2 = bNonPow2 ? "Optional (Full)" : "Conditional";
      shadows = bShadows ? c_szOptYes : c_szOptNo;
      instancing = bInstancing ? "Optional (Simple)" : c_szOptNo;
      cubeRT = bCubeRT ? c_szOptYes : c_szOptNo;
    }
    else if (pD3D11_1)
    {
      cb_partial = c_szYes;
      cb_offsetting = c_szYes;

      BOOL ext, x2, bpp565;
      CheckExtendedFormats(pD3D11_1, ext, x2, bpp565);

      bpp16 = (bpp565) ? c_szOptYes : c_szOptNo;

      bool bNonPow2, bShadows;
      CheckD3D9Ops(pD3D11_1, bNonPow2, bShadows);
      nonpow2 = bNonPow2 ? "Optional (Full)" : "Conditional";
      shadows = bShadows ? c_szOptYes : c_szOptNo;
    }
    break;

  default:
    return E_FAIL;
  }

  if (d3dType == D3D_DRIVER_TYPE_WARP)
  {
    maxTexDim = (pD3D12 || pD3D11_3 || pD3D11_2 || pD3D11_1) ? "16777216" : "65536";
  }

  if (pD3D12)
  {
    if (!vrs)
    {
      vrs = D3D12VRSSupported(pD3D12);
    }

    if (!meshShaders)
    {
      meshShaders = IsD3D12MeshShaderSupported(pD3D12) ? c_szOptYes : c_szOptNo;
    }

    if (!dxr)
    {
      dxr = D3D12DXRSupported(pD3D12);
    }
  }

  if (!pPrintInfo)
  {
    LVLINE("Shader Model", shaderModel);
    LVYESNO("Geometry Shader", (fl >= D3D_FEATURE_LEVEL_10_0));
    LVYESNO("Stream Out", (fl >= D3D_FEATURE_LEVEL_10_0));

    if (pD3D12 || pD3D11_3 || pD3D11_2 || pD3D11_1 || pD3D11)
    {
      if (g_dwViewState == IDM_VIEWALL || computeShader != c_szNo)
      {
        LVLINE("DirectCompute", computeShader);
      }

      LVYESNO("Hull & Domain Shaders", (fl >= D3D_FEATURE_LEVEL_11_0));
    }

    if (pD3D12)
    {
      LVLINE("Variable Rate Shading (VRS)", vrs);
      LVLINE("Mesh & Amplification Shaders", meshShaders);
      LVLINE("DirectX Raytracing", dxr);
    }

    LVYESNO("Texture Resource Arrays", (fl >= D3D_FEATURE_LEVEL_10_0));

    if (d3dVer != 0)
    {
      LVYESNO("Cubemap Resource Arrays", (fl >= D3D_FEATURE_LEVEL_10_1));
    }

    LVYESNO("BC4/BC5 Compression", (fl >= D3D_FEATURE_LEVEL_10_0));

    if (pD3D12 || pD3D11_3 || pD3D11_2 || pD3D11_1 || pD3D11)
    {
      LVYESNO("BC6H/BC7 Compression", (fl >= D3D_FEATURE_LEVEL_11_0));
    }

    LVYESNO("Alpha-to-coverage", (fl >= D3D_FEATURE_LEVEL_10_0));

    if (pD3D11_1 || pD3D11_2 || pD3D11_3 || pD3D12)
    {
      LVLINE("Logic Ops (Output Merger)", logic_ops);
    }

    if (pD3D11_1 || pD3D11_2 || pD3D11_3)
    {
      LVLINE("Constant Buffer Partial Updates", cb_partial);
      LVLINE("Constant Buffer Offsetting", cb_offsetting);

      if (uavEveryStage)
      {
        LVLINE("UAVs at Every Stage", uavEveryStage);
      }

      if (uavOnlyRender)
      {
        LVLINE("UAV-only rendering", uavOnlyRender);
      }
    }

    if (pD3D11_2 || pD3D11_3 || pD3D12)
    {
      if (tiled_rsc)
      {
        LVLINE("Tiled Resources", tiled_rsc);
      }
    }

    if (pD3D11_2 || pD3D11_3)
    {
      if (minmaxfilter)
      {
        LVLINE("Min/Max Filtering", minmaxfilter);
      }

      if (mapdefaultbuff)
      {
        LVLINE("Map DEFAULT Buffers", mapdefaultbuff);
      }
    }

    if (pD3D11_3 || pD3D12)
    {
      if (consrv_rast)
      {
        LVLINE("Conservative Rasterization", consrv_rast);
      }

      if (ps_stencil_ref)
      {
        LVLINE("PS-Specified Stencil Ref", ps_stencil_ref);
      }

      if (rast_ordered_views)
      {
        LVLINE("Rasterizer Ordered Views", rast_ordered_views);
      }
    }

    if (pD3D12 && binding_rsc)
    {
      LVLINE("Resource Binding", binding_rsc);
    }

    if (g_DXGIFactory1 && !pD3D12)
    {
      LVLINE("Extended Formats (BGRA, etc.)", extFormats);

      if (x2_10BitFormat && (g_dwViewState == IDM_VIEWALL || x2_10BitFormat != c_szNo))
      {
        LVLINE("10-bit XR High Color Format", x2_10BitFormat);
      }
    }

    if (pD3D11_1 || pD3D11_2 || pD3D11_3)
    {
      LVLINE("16-bit Formats (565/5551/4444)", bpp16);
    }

    if (nonpow2 && !pD3D12)
    {
      LVLINE("Non-Power-of-2 Textures", nonpow2);
    }

    LVLINE("Max Texture Dimension", maxTexDim);
    LVLINE("Max Cubemap Dimension", maxCubeDim);

    if (maxVolDim)
    {
      LVLINE("Max Volume Extent", maxVolDim);
    }

    if (maxTexRepeat)
    {
      LVLINE("Max Texture Repeat", maxTexRepeat);
    }

    LVLINE("Max Input Slots", maxInputSlots);

    if (uavSlots)
    {
      LVLINE("UAV Slots", uavSlots);
    }

    if (maxAnisotropy)
    {
      LVLINE("Max Anisotropy", maxAnisotropy);
    }

    LVLINE("Max Primitive Count", maxPrimCount);

    if (mrt)
    {
      LVLINE("Simultaneous Render Targets", mrt);
    }

    if (_10level9)
    {
      LVYESNO("Occlusion Queries", (fl >= D3D_FEATURE_LEVEL_9_2));
      LVYESNO("Separate Alpha Blend", (fl >= D3D_FEATURE_LEVEL_9_2));
      LVYESNO("Mirror Once", (fl >= D3D_FEATURE_LEVEL_9_2));
      LVYESNO("Overlapping Vertex Elements", (fl >= D3D_FEATURE_LEVEL_9_2));
      LVYESNO("Independant Write Masks", (fl >= D3D_FEATURE_LEVEL_9_3));

      LVLINE("Instancing", instancing);

      if (pD3D11_1 || pD3D11_2 || pD3D11_3)
      {
        LVLINE("Shadow Support", shadows);
      }

      if (pD3D11_2 || pD3D11_3)
      {
        LVLINE("Cubemap Render w/ non-Cube Depth", cubeRT);
      }
    }

    LVLINE("Note", FL_NOTE);
  }
  else
  {
    PRINTLINE("Shader Model", shaderModel);
    PRINTYESNO("Geometry Shader", (fl >= D3D_FEATURE_LEVEL_10_0));
    PRINTYESNO("Stream Out", (fl >= D3D_FEATURE_LEVEL_10_0));

    if (pD3D12 || pD3D11_3 || pD3D11_2 || pD3D11_1 || pD3D11)
    {
      PRINTLINE("DirectCompute", computeShader);
      PRINTYESNO("Hull & Domain Shaders", (fl >= D3D_FEATURE_LEVEL_11_0));
    }

    if (pD3D12)
    {
      PRINTLINE("Variable Rate Shading (VRS)", vrs);
      PRINTLINE("Mesh & Amplification Shaders", meshShaders);
      PRINTLINE("DirectX Raytracing", dxr);
    }

    PRINTYESNO("Texture Resource Arrays", (fl >= D3D_FEATURE_LEVEL_10_0));

    if (d3dVer != 0)
    {
      PRINTYESNO("Cubemap Resource Arrays", (fl >= D3D_FEATURE_LEVEL_10_1));
    }

    PRINTYESNO("BC4/BC5 Compression", (fl >= D3D_FEATURE_LEVEL_10_0));

    if (pD3D11_3 || pD3D11_2 || pD3D11_1 || pD3D11)
    {
      PRINTYESNO("BC6H/BC7 Compression", (fl >= D3D_FEATURE_LEVEL_11_0));
    }

    PRINTYESNO("Alpha-to-coverage", (fl >= D3D_FEATURE_LEVEL_10_0));

    if (pD3D11_1 || pD3D11_2 || pD3D11_3 || pD3D12)
    {
      PRINTLINE("Logic Ops (Output Merger)", logic_ops);
    }

    if (pD3D11_1 || pD3D11_2 || pD3D11_3)
    {
      PRINTLINE("Constant Buffer Partial Updates", cb_partial);
      PRINTLINE("Constant Buffer Offsetting", cb_offsetting);

      if (uavEveryStage)
      {
        PRINTLINE("UAVs at Every Stage", uavEveryStage);
      }

      if (uavOnlyRender)
      {
        PRINTLINE("UAV-only rendering", uavOnlyRender);
      }
    }

    if (pD3D11_2 || pD3D11_3 || pD3D12)
    {
      if (tiled_rsc)
      {
        PRINTLINE("Tiled Resources", tiled_rsc);
      }
    }

    if (pD3D11_2 || pD3D11_3)
    {
      if (minmaxfilter)
      {
        PRINTLINE("Min/Max Filtering", minmaxfilter);
      }

      if (mapdefaultbuff)
      {
        PRINTLINE("Map DEFAULT Buffers", mapdefaultbuff);
      }
    }

    if (pD3D11_3 || pD3D12)
    {
      if (consrv_rast)
      {
        PRINTLINE("Conservative Rasterization", consrv_rast);
      }

      if (ps_stencil_ref)
      {
        PRINTLINE("PS-Specified Stencil Ref ", ps_stencil_ref);
      }

      if (rast_ordered_views)
      {
        PRINTLINE("Rasterizer Ordered Views", rast_ordered_views);
      }
    }

    if (pD3D12 && binding_rsc)
    {
      PRINTLINE("Resource Binding", binding_rsc);
    }

    if (g_DXGIFactory1 && !pD3D12)
    {
      PRINTLINE("Extended Formats (BGRA, etc.)", extFormats);

      if (x2_10BitFormat)
      {
        PRINTLINE("10-bit XR High Color Format", x2_10BitFormat);
      }
    }

    if (pD3D11_1 || pD3D11_2 || pD3D11_3)
    {
      PRINTLINE("16-bit Formats (565/5551/4444)", bpp16);
    }

    if (nonpow2)
    {
      PRINTLINE("Non-Power-of-2 Textures", nonpow2);
    }

    PRINTLINE("Max Texture Dimension", maxTexDim);
    PRINTLINE("Max Cubemap Dimension", maxCubeDim);

    if (maxVolDim)
    {
      PRINTLINE("Max Volume Extent", maxVolDim);
    }

    if (maxTexRepeat)
    {
      PRINTLINE("Max Texture Repeat", maxTexRepeat);
    }

    PRINTLINE("Max Input Slots", maxInputSlots);

    if (uavSlots)
    {
      PRINTLINE("UAV Slots", uavSlots);
    }

    if (maxAnisotropy)
    {
      PRINTLINE("Max Anisotropy", maxAnisotropy);
    }

    PRINTLINE("Max Primitive Count", maxPrimCount);

    if (mrt)
    {
      PRINTLINE("Simultaneous Render Targets", mrt);
    }

    if (_10level9)
    {
      PRINTYESNO("Occlusion Queries", (fl >= D3D_FEATURE_LEVEL_9_2));
      PRINTYESNO("Separate Alpha Blend", (fl >= D3D_FEATURE_LEVEL_9_2));
      PRINTYESNO("Mirror Once", (fl >= D3D_FEATURE_LEVEL_9_2));
      PRINTYESNO("Overlapping Vertex Elements", (fl >= D3D_FEATURE_LEVEL_9_2));
      PRINTYESNO("Independant Write Masks", (fl >= D3D_FEATURE_LEVEL_9_3));

      PRINTLINE("Instancing", instancing);

      if (pD3D11_1 || pD3D11_2 || pD3D11_3)
      {
        PRINTLINE("Shadow Support", shadows);
      }

      if (pD3D11_2 || pD3D11_3)
      {
        PRINTLINE("Cubemap Render w/ non-Cube Depth", cubeRT);
      }
    }

    PRINTLINE("Note", FL_NOTE);
  }

  return S_OK;
}

#endif