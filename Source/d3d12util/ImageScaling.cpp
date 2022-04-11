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
#include "ImageScaling.h"
#include "scaler.h"
#include "CommandContext.h"


#include "CompiledShaders/DownScalingFilters.h"
#include "CompiledShaders/UpscalingFilters.h"
#include "CompiledShaders/ColorConvertNV12PS.h"
#include "CompiledShaders/Superxbr.h"
//#include "CompiledShaders/Superxbr3.h"
#include "compiledshaders/fxrcnnx.h"
#include "compiledshaders/fxrcnnx2.h"
#include "compiledshaders/fxrcnnx345.h"
#include "compiledshaders/fxrcnnx6.h"
#include "compiledshaders/fxrcnnx7.h"
#include "compiledshaders/fxrcnnx8.h"
#include "CompiledShaders/ScreenQuadPresentVS.h"
#include "CompiledShaders/VideoQuadPresentVS.h"
#include "CompiledShaders/BufferCopyPS.h"
#include "CompiledShaders/PresentSDRPS.h"
#include "CompiledShaders/PresentHDRPS.h"
#include "CompiledShaders/CompositeSDRPS.h"
#include "CompiledShaders/ScaleAndCompositeSDRPS.h"
#include "CompiledShaders/CompositeHDRPS.h"
#include "CompiledShaders/BlendUIHDRPS.h"
#include "CompiledShaders/ScaleAndCompositeHDRPS.h"
#include "D3D12PropPage.h"
//subpic
#include "CompiledShaders/vs_simple.h"
#include "CompiledShaders/ps_simple.h"
using namespace D3D12Engine;

namespace ImageScaling
{
  GraphicsPSO ColorConvertNV12PS(L"Image Scaling: Sharpen Upsample PSO");
  /*Scaling PSO*/
  GraphicsPSO fxrcnnxFilterPS(L"Image Scaling: fxrcnnx filter PSO pass#1");
  GraphicsPSO fxrcnnxFilter2PS(L"Image Scaling: fxrcnnx filter PSO pass#2");
  GraphicsPSO fxrcnnxFilter345PS(L"Image Scaling: fxrcnnx filter PSO pass#3 4 and 5");
  GraphicsPSO fxrcnnxFilter6PS(L"Image Scaling: fxrcnnx filter PSO pass#6");
  GraphicsPSO fxrcnnxFilter7PS(L"Image Scaling: fxrcnnx filter PSO pass#7");
  GraphicsPSO fxrcnnxFilter8PS(L"Image Scaling: fxrcnnx filter PSO pass#8");
  GraphicsPSO SuperXbrFiltersPS(L"Image Scaling: Xbr filter PSO");
  GraphicsPSO UpScalingFiltersPS(L"Image Scaling: Upscaling filters PSO");
  GraphicsPSO DownScalingFiltersPS(L"Image Scaling: DownScaling filters PSO");
  GraphicsPSO SubPicPS(L"SubPic PSO");
  GraphicsPSO VideoRessourceCopyPS(L"VideoRessourceCopy PSO");
  //constant buffer for downsampling
  CONSTANT_DOWNSCALE_BUFFER DownScalingConstantBuffer;;
  CONSTANT_UPSCALE_BUFFER UpScalingConstantBuffer;;

  /*Rendering PSO*/
  RootSignature s_PresentRSColor;
  RootSignature s_PresentRSScaling;
  RootSignature s_PresentRSfxrcnnxScaling;
  RootSignature s_SubPicRS;
  std::vector<CD3D12Scaler*> m_pScalers;
  CD3D12Scaler* scaler;
  //CD3D12Scaler* m_pDxScaler;
  CD3D12Scaler* m_pCubicScaler;
  CD3D12Scaler* m_pLanczos;
  CD3D12Scaler* m_pSpline;
  CD3D12Scaler* m_pJinc;
  CD3D12Scaler* m_pBilinearScaler;

  void Initialize(DXGI_FORMAT DestFormat)
  {
    /* Rendering and present PSOs*/
    s_PresentRSColor.Reset(4, 2);
    s_PresentRSColor[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
    s_PresentRSColor[1].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_ALL);
    s_PresentRSColor[2].InitAsBufferSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
    s_PresentRSColor[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
    s_PresentRSColor.InitStaticSampler(0, SamplerLinearClampDesc);
    s_PresentRSColor.InitStaticSampler(1, SamplerPointClampDesc);
    s_PresentRSColor.Finalize(L"Present");

    s_PresentRSScaling.Reset(4, 2);
    s_PresentRSScaling[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
    s_PresentRSScaling[1].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_ALL);
    s_PresentRSScaling[2].InitAsBufferSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
    s_PresentRSScaling[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
    s_PresentRSScaling.InitStaticSampler(0, SamplerLinearClampDesc);
    s_PresentRSScaling.InitStaticSampler(1, SamplerPointClampDesc);
    s_PresentRSScaling.Finalize(L"Present");

    s_PresentRSfxrcnnxScaling.Reset(4, 1);
    s_PresentRSfxrcnnxScaling[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 4);
    s_PresentRSfxrcnnxScaling[1].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_ALL);
    s_PresentRSfxrcnnxScaling[2].InitAsBufferSRV(4, D3D12_SHADER_VISIBILITY_PIXEL);
    s_PresentRSfxrcnnxScaling[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
    s_PresentRSfxrcnnxScaling.InitStaticSampler(0, SamplerfxrcnnxDesc);
    s_PresentRSfxrcnnxScaling.Finalize(L"Present");

    
    D3D12_SAMPLER_DESC SampDesc = {};
    SampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    SampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SampDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    SampDesc.MinLOD = 0;
    SampDesc.MaxLOD = D3D12_FLOAT32_MAX;
    s_SubPicRS.Reset(2, 1);
    s_SubPicRS.InitStaticSampler(0, SampDesc, D3D12_SHADER_VISIBILITY_PIXEL);
    s_SubPicRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
    s_SubPicRS[1].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_ALL);
    s_SubPicRS.Finalize(L"SupPic Renderer", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    D3D12_INPUT_ELEMENT_DESC vertElem[] =
    {
          { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,  0 },
          { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 }
    };

    SubPicPS.SetRootSignature(s_SubPicRS);
    SubPicPS.SetRasterizerState(D3D12Engine::RasterizerDefaultCw);
    SubPicPS.SetBlendState(D3D12Engine::BlendSubPic);
    SubPicPS.SetDepthStencilState(D3D12Engine::DepthStateDisabled);
    SubPicPS.SetInputLayout(_countof(vertElem), vertElem);
    SubPicPS.SetSampleMask(0xFFFFFFFF);
    SubPicPS.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    SubPicPS.SetVertexShader(g_pvs_simple, sizeof(g_pvs_simple));
    SubPicPS.SetPixelShader(g_pps_simple, sizeof(g_pps_simple));
    SubPicPS.SetRenderTargetFormat(DestFormat, DXGI_FORMAT_UNKNOWN);
    SubPicPS.Finalize();

    VideoRessourceCopyPS.SetRootSignature(s_SubPicRS);
    VideoRessourceCopyPS.SetRasterizerState(D3D12Engine::RasterizerDefaultCw);
    VideoRessourceCopyPS.SetBlendState(D3D12Engine::BlendDisable);
    VideoRessourceCopyPS.SetDepthStencilState(D3D12Engine::DepthStateDisabled);
    VideoRessourceCopyPS.SetInputLayout(_countof(vertElem), vertElem);
    VideoRessourceCopyPS.SetSampleMask(0xFFFFFFFF);
    VideoRessourceCopyPS.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    VideoRessourceCopyPS.SetVertexShader(g_pvs_simple, sizeof(g_pvs_simple));
    VideoRessourceCopyPS.SetPixelShader(g_pps_simple, sizeof(g_pps_simple));
    VideoRessourceCopyPS.SetRenderTargetFormat(DestFormat, DXGI_FORMAT_UNKNOWN);
    VideoRessourceCopyPS.Finalize();

    DownScalingFiltersPS.SetRootSignature(s_PresentRSScaling);
    DownScalingFiltersPS.SetRasterizerState(D3D12Engine::RasterizerDefault);
    DownScalingFiltersPS.SetBlendState(D3D12Engine::BlendDisable);
    DownScalingFiltersPS.SetDepthStencilState(D3D12Engine::DepthStateDisabled);
    DownScalingFiltersPS.SetSampleMask(0xFFFFFFFF);
    DownScalingFiltersPS.SetInputLayout(0, nullptr);
    DownScalingFiltersPS.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    DownScalingFiltersPS.SetVertexShader(g_pScreenQuadPresentVS, sizeof(g_pScreenQuadPresentVS));
    DownScalingFiltersPS.SetPixelShader(g_pDownScalingFilters, sizeof(g_pDownScalingFilters));
    DownScalingFiltersPS.SetRenderTargetFormat(DestFormat, DXGI_FORMAT_UNKNOWN);
    DownScalingFiltersPS.Finalize();

    UpScalingFiltersPS.SetRootSignature(s_PresentRSScaling);
    UpScalingFiltersPS.SetRasterizerState(D3D12Engine::RasterizerDefault);
    UpScalingFiltersPS.SetBlendState(D3D12Engine::BlendDisable);
    UpScalingFiltersPS.SetDepthStencilState(D3D12Engine::DepthStateDisabled);
    UpScalingFiltersPS.SetSampleMask(0xFFFFFFFF);
    UpScalingFiltersPS.SetInputLayout(0, nullptr);
    UpScalingFiltersPS.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    UpScalingFiltersPS.SetVertexShader(g_pScreenQuadPresentVS, sizeof(g_pScreenQuadPresentVS));
    UpScalingFiltersPS.SetPixelShader(g_pUpScalingFilters, sizeof(g_pUpScalingFilters));
    UpScalingFiltersPS.SetRenderTargetFormat(DestFormat, DXGI_FORMAT_UNKNOWN);
    UpScalingFiltersPS.Finalize();

    SuperXbrFiltersPS.SetRootSignature(s_PresentRSScaling);
    SuperXbrFiltersPS.SetRasterizerState(D3D12Engine::RasterizerDefault);
    SuperXbrFiltersPS.SetBlendState(D3D12Engine::BlendDisable);
    SuperXbrFiltersPS.SetDepthStencilState(D3D12Engine::DepthStateDisabled);
    SuperXbrFiltersPS.SetSampleMask(0xFFFFFFFF);
    SuperXbrFiltersPS.SetInputLayout(0, nullptr);
    SuperXbrFiltersPS.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    SuperXbrFiltersPS.SetVertexShader(g_pScreenQuadPresentVS, sizeof(g_pScreenQuadPresentVS));
    SuperXbrFiltersPS.SetPixelShader(g_psuperxbr, sizeof(g_psuperxbr));
    SuperXbrFiltersPS.SetRenderTargetFormat(DestFormat, DXGI_FORMAT_UNKNOWN);
    SuperXbrFiltersPS.Finalize();

    fxrcnnxFilterPS.SetRootSignature(s_PresentRSfxrcnnxScaling);
    fxrcnnxFilterPS.SetRasterizerState(D3D12Engine::RasterizerDefault);
    fxrcnnxFilterPS.SetBlendState(D3D12Engine::Blendfxrcnnx);
    fxrcnnxFilterPS.SetDepthStencilState(D3D12Engine::DepthStateDisabled);
    fxrcnnxFilterPS.SetSampleMask(0xFFFFFFFF);
    fxrcnnxFilterPS.SetInputLayout(0, nullptr);
    fxrcnnxFilterPS.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    fxrcnnxFilterPS.SetVertexShader(g_pScreenQuadPresentVS, sizeof(g_pScreenQuadPresentVS));
    fxrcnnxFilterPS.SetPixelShader(g_pfxrcnnx, sizeof(g_pfxrcnnx));
    //DXGI_FORMAT formats[3] = { DXGI_FORMAT_B8G8R8A8_UNORM ,DXGI_FORMAT_R16G16B16A16_FLOAT ,DestFormat };
    fxrcnnxFilterPS.SetRenderTargetFormat(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    
    fxrcnnxFilter2PS.SetRootSignature(s_PresentRSfxrcnnxScaling);
    fxrcnnxFilter2PS.SetRasterizerState(D3D12Engine::RasterizerDefault);
    fxrcnnxFilter2PS.SetBlendState(D3D12Engine::Blendfxrcnnx);
    fxrcnnxFilter2PS.SetDepthStencilState(D3D12Engine::DepthStateDisabled);
    fxrcnnxFilter2PS.SetSampleMask(0xFFFFFFFF);
    fxrcnnxFilter2PS.SetInputLayout(0, nullptr);
    fxrcnnxFilter2PS.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    fxrcnnxFilter2PS.SetVertexShader(g_pScreenQuadPresentVS, sizeof(g_pScreenQuadPresentVS));
    fxrcnnxFilter2PS.SetPixelShader(g_pfxrcnnx2, sizeof(g_pfxrcnnx2));
    DXGI_FORMAT formats[2] = { DXGI_FORMAT_R16G16B16A16_FLOAT ,DXGI_FORMAT_R16G16B16A16_FLOAT };
    fxrcnnxFilter2PS.SetRenderTargetFormats(2, formats, DXGI_FORMAT_UNKNOWN);
    
    
    fxrcnnxFilter345PS.SetRootSignature(s_PresentRSfxrcnnxScaling);
    fxrcnnxFilter345PS.SetRasterizerState(D3D12Engine::RasterizerDefault);
    fxrcnnxFilter345PS.SetBlendState(D3D12Engine::Blendfxrcnnx);
    fxrcnnxFilter345PS.SetDepthStencilState(D3D12Engine::DepthStateDisabled);
    fxrcnnxFilter345PS.SetSampleMask(0xFFFFFFFF);
    fxrcnnxFilter345PS.SetInputLayout(0, nullptr);
    fxrcnnxFilter345PS.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    fxrcnnxFilter345PS.SetVertexShader(g_pScreenQuadPresentVS, sizeof(g_pScreenQuadPresentVS));
    fxrcnnxFilter345PS.SetPixelShader(g_pfxrcnnx345, sizeof(g_pfxrcnnx345));
    fxrcnnxFilter345PS.SetRenderTargetFormats(2, formats, DXGI_FORMAT_UNKNOWN);
    
    fxrcnnxFilter6PS = fxrcnnxFilter345PS;
    fxrcnnxFilter6PS.SetName(L"Image Scaling: fxrcnnx filter PSO pass#6");
    fxrcnnxFilter6PS.SetPixelShader(g_pfxrcnnx6, sizeof(g_pfxrcnnx6));
    
    fxrcnnxFilter7PS = fxrcnnxFilterPS;
    fxrcnnxFilter7PS.SetName(L"Image Scaling: fxrcnnx filter PSO pass#7");
    fxrcnnxFilter7PS.SetPixelShader(g_pfxrcnnx7, sizeof(g_pfxrcnnx7));
    fxrcnnxFilter7PS.SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN);
    
    fxrcnnxFilter8PS = fxrcnnxFilterPS;
    fxrcnnxFilter8PS.SetName(L"Image Scaling: fxrcnnx filter PSO pass#8");
    DXGI_FORMAT formatsYuvWith16[2] = { DXGI_FORMAT_B8G8R8A8_UNORM ,DXGI_FORMAT_R16G16B16A16_FLOAT };
    fxrcnnxFilter8PS.SetRenderTargetFormat(DestFormat,DXGI_FORMAT_UNKNOWN);
    fxrcnnxFilter8PS.SetPixelShader(g_pfxrcnnx8, sizeof(g_pfxrcnnx8));
    fxrcnnxFilterPS.Finalize();
    fxrcnnxFilter2PS.Finalize();
    fxrcnnxFilter345PS.Finalize();
    fxrcnnxFilter6PS.Finalize();
    fxrcnnxFilter7PS.Finalize();
    fxrcnnxFilter8PS.Finalize();
    


    ColorConvertNV12PS.SetRootSignature(s_PresentRSColor);
    ColorConvertNV12PS.SetRasterizerState(D3D12Engine::RasterizerDefault);
    ColorConvertNV12PS.SetBlendState(D3D12Engine::BlendDisable);
    ColorConvertNV12PS.SetDepthStencilState(D3D12Engine::DepthStateDisabled);
    ColorConvertNV12PS.SetSampleMask(0xFFFFFFFF);
    ColorConvertNV12PS.SetInputLayout(0, nullptr);
    ColorConvertNV12PS.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    ColorConvertNV12PS.SetVertexShader(g_pScreenQuadPresentVS, sizeof(g_pScreenQuadPresentVS));
    ColorConvertNV12PS.SetPixelShader(g_pColorConvertNV12PS, sizeof(g_pColorConvertNV12PS));
    ColorConvertNV12PS.SetRenderTargetFormat(DestFormat, DXGI_FORMAT_UNKNOWN);
    ColorConvertNV12PS.Finalize();

    CD3D12Scaler* scaler = new CD3D12Scaler(L"superxbr");
    ScalerConfigInt cint;
    cint.Name = L"pass";
    cint.Value = 0;

    scaler->g_ScalerInternalInt.push_back(cint);
    cint.Name = L"fastmethod";
    scaler->g_ScalerInternalInt.push_back(cint);
    m_pScalers.push_back(scaler);
    scaler = new CD3D12Scaler(L"fxrcnnx");
    cint.Name = L"pass";
    cint.Value = 0;
    scaler->g_ScalerInternalInt.push_back(cint);
    m_pScalers.push_back(scaler);
  }

  CD3D12Scaler* GetScaler(std::wstring name)
  {
    for (CD3D12Scaler* i : m_pScalers)
    {
      if (i->Name() == name)
      {
        return i;
      }
    }
    assert(0);
    return nullptr;
  }

  HRESULT CreateVertex(const UINT srcW, const UINT srcH, const RECT& srcRect, VERTEX_SUBPIC vertices[4])
  {
    
    const float src_dx = 1.0f / srcW;
    const float src_dy = 1.0f / srcH;
    float src_l = src_dx * srcRect.left;
    float src_r = src_dx * srcRect.right;
    const float src_t = src_dy * srcRect.top;
    const float src_b = src_dy * srcRect.bottom;

    POINT points[4];
    points[0] = { -1, -1 };
    points[1] = { -1, +1 };
    points[2] = { +1, -1 };
    points[3] = { +1, +1 };

    vertices[0].Pos = { (float)points[0].x, (float)points[0].y, 0 };
    vertices[0].TexCoord = { src_l, src_b };
    vertices[1].Pos = { (float)points[1].x, (float)points[1].y, 0 };
    vertices[1].TexCoord = { src_l, src_t };
    vertices[2].Pos = { (float)points[2].x, (float)points[2].y, 0 };
    vertices[2].TexCoord = { src_r, src_b };
    vertices[3].Pos = { (float)points[3].x, (float)points[3].y, 0 };
    vertices[3].TexCoord = { src_r, src_t };
    
    return S_OK;
  }

  HRESULT RenderToBackBuffer(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source)
  {

    Context.SetRootSignature(s_SubPicRS);
    Context.SetPipelineState(VideoRessourceCopyPS);
    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);// D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    Context.TransitionResource(dest, D3D12_RESOURCE_STATE_RENDER_TARGET);
    Context.SetRenderTarget(dest.GetRTV());


    Context.TransitionResource(source, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    Context.SetViewportAndScissor(0, 0, dest.GetWidth(), dest.GetHeight());
    Context.SetDynamicDescriptor(0, 0, source.GetSRV());
    VERTEX_SUBPIC subpicvertex[4];
    
    CRect rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = dest.GetWidth();
    rect.bottom = dest.GetHeight();
    CreateVertex(dest.GetWidth(), dest.GetHeight(), rect, subpicvertex);
    Context.SetDynamicVB(0, 4, sizeof(VERTEX_SUBPIC), subpicvertex);
    //Context.SetDynamicDescriptor(0, 1, target.GetSRV());


    Context.SetViewportAndScissor(rect.left, rect.top, rect.Width(), rect.Height());

    Context.Draw(4);
    
    return S_OK;
  }
  
  HRESULT RenderAlphaBitmap(GraphicsContext& Context, Texture& resource, RECT alpharect)
  {

    Context.SetRootSignature(s_SubPicRS);
    Context.SetPipelineState(SubPicPS);
    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);// D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Context.SetDynamicDescriptor(0, 0, resource.GetSRV());
    VERTEX_SUBPIC subpicvertex[4];
    CreateVertex(resource.GetWidth(), resource.GetHeight(), alpharect, subpicvertex);
    Context.SetDynamicVB(0, 4, sizeof(VERTEX_SUBPIC), subpicvertex);
    Context.Draw(4);
    return S_OK;
  }
  
  HRESULT RenderSubPic(GraphicsContext& Context, ColorBuffer& resource, ColorBuffer& target, CRect srcRect, UINT srcW, UINT srcH)
  {
    
    Context.SetRootSignature(s_SubPicRS);
    Context.SetPipelineState(SubPicPS);
    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);// D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    Context.TransitionResource(target, D3D12_RESOURCE_STATE_RENDER_TARGET);
    Context.SetRenderTarget(target.GetRTV());


    Context.TransitionResource(resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    Context.SetViewportAndScissor(0, 0, target.GetWidth(), target.GetHeight());
    Context.SetDynamicDescriptor(0, 0, resource.GetSRV());
    VERTEX_SUBPIC subpicvertex[4];
    CreateVertex(srcW, srcH, srcRect, subpicvertex);
    Context.SetDynamicVB(0, 4, sizeof(VERTEX_SUBPIC), subpicvertex);
    //Context.SetDynamicDescriptor(0, 1, target.GetSRV());
    
    
    Context.SetViewportAndScissor(srcRect.left, srcRect.top, srcRect.Width(), srcRect.Height());
    Context.TransitionResource(target, D3D12_RESOURCE_STATE_RENDER_TARGET);
    Context.Draw(4);
    
    return S_OK;
  }

  void CopyPlanesSW(GraphicsContext& Context, ColorBuffer& dest, Texture& plane0, Texture& plane1, CONSTANT_BUFFER_VAR& colorconstant, CRect destRect)
  {
    Context.SetRootSignature(s_PresentRSColor);
    Context.SetPipelineState(ColorConvertNV12PS);
    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Context.SetDynamicConstantBufferView(1, sizeof(CONSTANT_BUFFER_VAR), &colorconstant);

    Context.TransitionResource(plane0, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context.TransitionResource(plane1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context.TransitionResource(dest, D3D12_RESOURCE_STATE_RENDER_TARGET);
    Context.SetRenderTarget(dest.GetRTV());
    if (destRect.Width() == 0 && destRect.Height() == 0)
      Context.SetViewportAndScissor(0, 0, dest.GetWidth(), dest.GetHeight());
    else
      Context.SetViewportAndScissor(destRect.left, destRect.top, destRect.Width(), destRect.Height());
    Context.SetDynamicDescriptor(0, 0, plane0.GetSRV());
    Context.SetDynamicDescriptor(0, 1, plane1.GetSRV());
    Context.Draw(3);
    
  }

  void ColorAjust(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source0, ColorBuffer& source1, CONSTANT_BUFFER_VAR& colorconstant, CRect destRect)
  {

    Context.SetRootSignature(s_PresentRSColor);
    Context.SetPipelineState(ColorConvertNV12PS);
    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Context.SetDynamicConstantBufferView(1, sizeof(CONSTANT_BUFFER_VAR), &colorconstant);
    
    Context.TransitionResource(source0, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context.TransitionResource(source1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context.TransitionResource(dest, D3D12_RESOURCE_STATE_RENDER_TARGET);
    Context.SetRenderTarget(dest.GetRTV());
    if (destRect.Width() == 0 && destRect.Height() == 0)
      Context.SetViewportAndScissor(0, 0, dest.GetWidth(), dest.GetHeight());
    else
      Context.SetViewportAndScissor(destRect.left, destRect.top, destRect.Width(), destRect.Height());
    Context.SetDynamicDescriptor(0, 0, source0.GetSRV());
    Context.SetDynamicDescriptor(0, 1, source1.GetSRV());
    Context.Draw(3);
  }

  void Downscale(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, CRect srcRect, CRect destRect)
  {
    //the viewport x 0 y 46 width the width and height of the targetvideo
    Context.SetRootSignature(s_PresentRSScaling);
    Context.SetPipelineState(DownScalingFiltersPS);
    Context.TransitionResource(dest, D3D12_RESOURCE_STATE_RENDER_TARGET);
    Context.TransitionResource(source, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Context.SetRenderTarget(dest.GetRTV());
    Context.SetViewportAndScissor(destRect.left, destRect.top, destRect.Width(), destRect.Height());
    Context.SetDynamicDescriptor(0, 0, source.GetSRV());

    DownScalingConstantBuffer.wh = { (float)srcRect.Width(),(float)srcRect.Height() };
    DownScalingConstantBuffer.dxdy = { 1.0f / srcRect.Width(),1.0f / srcRect.Height() };
    DownScalingConstantBuffer.scale = { (float)srcRect.Width() / destRect.Width(), (float)srcRect.Height() / destRect.Height() };

    float filter_support = 1.0f;
    
    switch (g_D3D12Options->GetCurrentDownscaler())
    {
    case 0://ImageScaling::kDownBox:
      filter_support = 0.5f;
      break;
    case 1://ImageScaling::kDownBilinear:
      filter_support = 1.0f;
      break;
    case 2://ImageScaling::kDownHamming:
      filter_support = 1.0f;
      break;
    case 3://ImageScaling::kDownBicubic:
      filter_support = 2.0;
      break;
    case 4://ImageScaling::kDownLanczos:
      filter_support = 3.0;
      break;
    }
    if (DownScalingConstantBuffer.scale.x == 1.0f)
    {
      DownScalingConstantBuffer.axis = 0;
      DownScalingConstantBuffer.support = filter_support * DownScalingConstantBuffer.scale.x;
      DownScalingConstantBuffer.ss = 1.0f / DownScalingConstantBuffer.scale.x;
    }
    else
    {
      DownScalingConstantBuffer.axis = 1;
      DownScalingConstantBuffer.support = filter_support * DownScalingConstantBuffer.scale.y;
      DownScalingConstantBuffer.ss = 1.0f / DownScalingConstantBuffer.scale.y;
    }
    DownScalingConstantBuffer.filter = g_D3D12Options->GetCurrentDownscaler();
    Context.SetDynamicConstantBufferView(1, sizeof(CONSTANT_DOWNSCALE_BUFFER), &DownScalingConstantBuffer);

    Context.Draw(3);
  }

  
  void UpscaleXbr(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, CRect srcRect, CRect destRect)
  {
    //xbr has 2 interal config done on the shader side passes and fast method
    //3 config on user side 2 in the shader strength and sharpness one in the client side which is the factor
    //iArgs[0] str
    //fArgs[] sharp
    Context.SetRootSignature(s_PresentRSScaling);
    Context.SetPipelineState(SuperXbrFiltersPS);
    //need to reset the pass
    CScalerOption* opt = D3D12Engine::g_D3D12Options->GetScaler("superxbr");
    CD3D12Scaler* scaler = GetScaler(L"superxbr");
    int w, h, w2,h2;
    
    w = srcRect.Width() * 2;
    h = srcRect.Height() * 2;
    
    int fact = s_factor[opt->GetInt("factor")];
    int iArgs[4] = {};
    float fArgs[4] = {};
    iArgs[0] = opt->GetInt("strength");
    fArgs[0] = opt->GetFloat("sharp");
    //fast method
    scaler->g_ScalerInternalInt[1].Value = 0;
    //int value 0 is pass
    scaler->g_ScalerInternalInt[0].Value = 0;// push_back(cint); //SetConfigInt(L"pass", 0);
    Context.SetViewportAndScissor(0, 0, dest.GetWidth(), dest.GetHeight());
    Context.SetDynamicDescriptor(0, 0, source.GetSRV());
    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Context.SetRenderTarget(dest.GetRTV());
    
    scaler->ShaderPass(Context, dest, source, (w/2), (h/2),iArgs,fArgs);
    Context.SetDynamicDescriptor(0, 0, dest.GetSRV());
    scaler->g_ScalerInternalInt[0].Value = 1;
    scaler->ShaderPass(Context, dest, source, w , h, iArgs, fArgs);
    scaler->g_ScalerInternalInt[0].Value = 2;
    scaler->ShaderPass(Context, dest, source, w, h, iArgs, fArgs);

    if (fact >= 4)
    {
      w2 = w * 4;
      h2 = h * 4;
      scaler->g_ScalerInternalInt[0].Value = 0;
      scaler->ShaderPass(Context, dest, source, (w2/2), (h2/2), iArgs, fArgs);
      scaler->g_ScalerInternalInt[0].Value = 1;
      scaler->ShaderPass(Context, dest, source, w2,h2, iArgs, fArgs);
      scaler->g_ScalerInternalInt[0].Value = 2;
      scaler->ShaderPass(Context, dest, source, w2,h2, iArgs, fArgs);
    }

    if (fact >= 8)
    {
      w2 = w * 8;
      h2 = h * 8;
      scaler->g_ScalerInternalInt[0].Value = 0;
      scaler->ShaderPass(Context, dest, source, (w2 / 2), (h2 / 2), iArgs, fArgs);
      scaler->g_ScalerInternalInt[0].Value = 1;
      scaler->ShaderPass(Context, dest, source, w2, h2, iArgs, fArgs);
      scaler->g_ScalerInternalInt[0].Value = 2;
      scaler->ShaderPass(Context, dest, source, w2, h2, iArgs, fArgs);
    }

    if (fact >= 16)
    {
      w2 = w * 16;
      h2 = h * 16;
      scaler->g_ScalerInternalInt[0].Value = 0;
      scaler->ShaderPass(Context, dest, source, (w2 / 2), (h2 / 2), iArgs, fArgs);
      scaler->g_ScalerInternalInt[0].Value = 1;
      scaler->ShaderPass(Context, dest, source, w2, h2, iArgs, fArgs);
      scaler->g_ScalerInternalInt[0].Value = 2;
      scaler->ShaderPass(Context, dest, source, w2, h2, iArgs, fArgs);
    }
    scaler->Done();
    g_D3D12Options->SetCurrentUpscaler(4);
    Upscale(Context, dest, source, srcRect, destRect);
    g_D3D12Options->SetCurrentUpscaler(6);
    //Upscale(Context, dest, dest, 2, srcRect, destRect);
    //todo add scaler at the end in the config right now we only do cubic
    //Upscale(Context, dest, dest, 2, srcRect, destRect);
  }

  void Upscalefxrcnnx(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, CRect srcRect, CRect destRect)
  {
    
    CScalerOption* opt = D3D12Engine::g_D3D12Options->GetScaler("superxbr");
    CD3D12Scaler* scaler = GetScaler(L"fxrcnnx");
    int w, h, w2, h2;

    w = srcRect.Width() * 2;
    h = srcRect.Height() * 2;

    int fact = s_factor[opt->GetInt("factor")];
    int iArgs[4] = {};
    float fArgs[4] = {};
    iArgs[0] = opt->GetInt("strength");
    fArgs[0] = opt->GetFloat("sharp");

    /*Crate teture required for fxrcnnx if not already created */
    if (!scaler->g_bTextureCreated)
    {
      scaler->CreateTexture(L"yuvtex", srcRect, DXGI_FORMAT_B8G8R8A8_UNORM);
      scaler->CreateTexture(L"featuremap1", srcRect, DXGI_FORMAT_R16G16B16A16_FLOAT);
      scaler->CreateTexture(L"featuremap2", srcRect, DXGI_FORMAT_R16G16B16A16_FLOAT);
      scaler->CreateTexture(L"tex1", srcRect, DXGI_FORMAT_R16G16B16A16_FLOAT);
      scaler->CreateTexture(L"tex2", srcRect, DXGI_FORMAT_R16G16B16A16_FLOAT);
      scaler->CreateTexture(L"tex3", srcRect, DXGI_FORMAT_R16G16B16A16_FLOAT);
      scaler->CreateTexture(L"tex4", srcRect, DXGI_FORMAT_R16G16B16A16_FLOAT);
    }
    

    //int value 0 is pass
    scaler->g_ScalerInternalInt[0].Value = 0;

    Context.SetRootSignature(s_PresentRSfxrcnnxScaling);
    Context.SetPipelineState(fxrcnnxFilterPS);
    
    Context.SetViewportAndScissor(0, 0, source.GetWidth(), source.GetHeight());
    Context.SetDynamicDescriptor(0, 0, source.GetSRV());
    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    //pass1 source in >> yuvtex out
    scaler->SetRenderTargets(Context, { L"yuvtex" },true);
    scaler->ShaderPass(Context, dest, source, w, h, iArgs, fArgs);
    
    Context.SetPipelineState(fxrcnnxFilter2PS);
    scaler->SetTextureSrv(Context, L"yuvtex", 0, 0);
    scaler->SetRenderTargets(Context, {L"featuremap1",L"featuremap2"},true);
    //scaler->SetTextureSrv(Context, L"texturemap1", 0, 2);
    scaler->g_ScalerInternalInt[0].Value = 1;
    //magpie calculation for vars effectdrawer.cpp
#if 0
    exprParser.DefineVar("INPUT_WIDTH", &inputWidth);
    exprParser.DefineVar("INPUT_HEIGHT", &inputHeight);
    exprParser.DefineVar("INPUT_PT_X", &inputPtX);
    exprParser.DefineVar("INPUT_PT_Y", &inputPtY);
    exprParser.DefineVar("OUTPUT_WIDTH", &outputWidth);
    exprParser.DefineVar("OUTPUT_HEIGHT", &outputHeight);
    exprParser.DefineVar("OUTPUT_PT_X", &outputPtX);
    exprParser.DefineVar("OUTPUT_PT_Y", &outputPtY);
    exprParser.DefineVar("SCALE_X", &scaleX);
    exprParser.DefineVar("SCALE_Y", &scaleY);
    inputWidth = inputSize.cx;
  inputHeight = inputSize.cy;
  inputPtX = 1.0f / inputSize.cx;
  inputPtY = 1.0f / inputSize.cy;
  outputWidth = outputSize.cx;
  outputHeight = outputSize.cy;
  outputPtX = 1.0f / outputSize.cx;
  outputPtY = 1.0f / outputSize.cy;
  scaleX = inputPtX * outputWidth;
  scaleY = inputPtY * outputHeight;
#endif
    fArgs[0] = 1.0f / (float)source.GetWidth();
    fArgs[1] = 1.0f / (float)source.GetHeight();
    
    //pass2 yuvtex in >> featuremap1 and featuremap2 out
    scaler->ShaderPass(Context, dest, source, w, h, iArgs, fArgs);

    Context.SetPipelineState(fxrcnnxFilter345PS);
    scaler->SetRenderTargets(Context, { L"tex1",L"tex2" }, true);
    scaler->SetTextureSrv(Context, L"featuremap1", 0, 0);
    scaler->SetTextureSrv(Context, L"featuremap2", 0, 1,true);
    scaler->g_ScalerInternalInt[0].Value = 3;
    //pass3 featuremap1 and featuremap2 in >> tex1 and tex2 out
    scaler->ShaderPass(Context, dest, source, w, h, iArgs, fArgs);

    // pass 4 and 5 as 3
    //pass4 tex1 and tex2 in >> tex3 and tex4 out
    scaler->SetTextureSrv(Context, L"tex1", 0, 0);
    scaler->SetTextureSrv(Context, L"tex2", 0, 1, true);
    scaler->SetRenderTargets(Context, { L"tex3",L"tex4" }, true);
    scaler->g_ScalerInternalInt[0].Value = 4;//set pass 4
    scaler->ShaderPass(Context, dest, source, w, h, iArgs, fArgs);
//pass5 tex3 and tex4 in >> tex1 and tex2 out
    scaler->SetTextureSrv(Context, L"tex3", 0, 0);
    scaler->SetTextureSrv(Context, L"tex4", 0, 1, true);
    scaler->SetRenderTargets(Context, { L"tex1",L"tex2" }, true);
    scaler->g_ScalerInternalInt[0].Value = 5;//set pass 5
    scaler->ShaderPass(Context, dest, source, w, h, iArgs, fArgs);
//pass6 tex1,tex2,featuremap1,featuremap2 in >> tex3 and tex4 out
    Context.SetPipelineState(fxrcnnxFilter6PS);
    scaler->SetTextureSrv(Context, L"tex1", 0, 0);
    scaler->SetTextureSrv(Context, L"tex2", 0, 1);
    scaler->SetTextureSrv(Context, L"featuremap1", 0, 2);
    scaler->SetTextureSrv(Context, L"featuremap2", 0, 3);
    scaler->SetRenderTargets(Context, { L"tex3",L"tex4" }, true);
    scaler->ShaderPass(Context, dest, source, w, h, iArgs, fArgs);
//pass7 tex3,tex4 in >> tex1 out
    Context.SetPipelineState(fxrcnnxFilter7PS);
    scaler->SetTextureSrv(Context, L"tex3", 0, 0);
    scaler->SetTextureSrv(Context, L"tex4", 0, 1);
    scaler->SetRenderTargets(Context, { L"tex1" }, true);
    scaler->ShaderPass(Context, dest, source, w, h, iArgs, fArgs);
//pass8 yuvtex ,tex1 in >> no save out
    Context.SetPipelineState(fxrcnnxFilter8PS);
    scaler->SetTextureSrv(Context, L"yuvtex", 0, 0);
    scaler->SetTextureSrv(Context, L"tex1", 0, 1);
    Context.SetRenderTarget(dest.GetRTV());
    Context.SetViewportAndScissor(destRect.left, destRect.top, destRect.Width(), destRect.Height());
    //Context.SetViewportAndScissor(0, 0, dest.GetWidth(), dest.GetHeight());
    scaler->ShaderPass(Context, dest, source, w, h, iArgs, fArgs);
    Context.SetRootSignature(s_PresentRSScaling);
    //Context.SetPipelineState(fxrcnnxFilterPS);
    
  }

  void Upscale(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, CRect srcRect, CRect destRect)
  {
    Context.SetRootSignature(s_PresentRSScaling);
    Context.SetPipelineState(UpScalingFiltersPS);
    Context.TransitionResource(dest, D3D12_RESOURCE_STATE_RENDER_TARGET);
    if (dest.GetSRV().ptr != source.GetSRV().ptr)
      Context.TransitionResource(source, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Context.SetRenderTarget(dest.GetRTV());
    Context.SetViewportAndScissor(destRect.left, destRect.top, destRect.Width(), destRect.Height());
    Context.SetDynamicDescriptor(0, 0, source.GetSRV());

    UpScalingConstantBuffer.wh = { (float)srcRect.Width(),(float)srcRect.Height() };
    UpScalingConstantBuffer.dxdy = { 1.0f / srcRect.Width(),1.0f / srcRect.Height() };
    UpScalingConstantBuffer.scale = { (float)srcRect.Width() / destRect.Width(), (float)srcRect.Height() / destRect.Height() };

    if (UpScalingConstantBuffer.scale.x == 1.0f)
      UpScalingConstantBuffer.axis = 0;
    else
      UpScalingConstantBuffer.axis = 1;

    UpScalingConstantBuffer.filter = g_D3D12Options->GetCurrentUpscaler();
    if (UpScalingConstantBuffer.filter == 3)
      UpScalingConstantBuffer.lanczostype = g_D3D12Options->GetScaler("lanczos")->GetInt("lanczostype");
    if (UpScalingConstantBuffer.filter == 4)
      UpScalingConstantBuffer.splinetype = g_D3D12Options->GetScaler("spline")->GetInt("splinetype");
    //jync
    if (UpScalingConstantBuffer.filter == 5)
    {
//#pragma parameter JINC2_WINDOW_SINC "Window Sinc Param" 0.44 0.0 1.0 0.01
//#pragma parameter JINC2_SINC "Sinc Param" 0.82 0.0 1.0 0.01
//#pragma parameter JINC2_AR_STRENGTH "Anti-ringing Strength" 0.5 0.0 1.0 0.1
      CScalerOption* opt = g_D3D12Options->GetScaler("jinc");
      UpScalingConstantBuffer.jinc2.x = 0.5f;// s_jincwindowsinc[opt->GetInt("windowsinc")];
      UpScalingConstantBuffer.jinc2.y = 0.5f;//s_jincsinc[opt->GetInt("sinc")];
      UpScalingConstantBuffer.jinc2.z = 0.5f;//s_jincsinc[opt->GetInt("str")];
    }
    Context.SetDynamicConstantBufferView(1, sizeof(CONSTANT_UPSCALE_BUFFER), &UpScalingConstantBuffer);
    Context.Draw(3);
  }

  void FreeImageScaling()
  {
    /*We need to reset them to 0 if we dont the root signature is not the same when we start a new video*/
    s_PresentRSColor.Free();
    s_PresentRSScaling.Free();
    s_PresentRSfxrcnnxScaling.Free();
    s_SubPicRS.Free();
    ColorConvertNV12PS.FreePSO();
    UpScalingFiltersPS.FreePSO();
    SuperXbrFiltersPS.FreePSO();
    DownScalingFiltersPS.FreePSO();
    SubPicPS.FreePSO();
    VideoRessourceCopyPS.FreePSO();
    m_pScalers.clear();
    fxrcnnxFilterPS.FreePSO();
    fxrcnnxFilter2PS.FreePSO();
    fxrcnnxFilter345PS.FreePSO();
    fxrcnnxFilter6PS.FreePSO();
    fxrcnnxFilter7PS.FreePSO();
    fxrcnnxFilter8PS.FreePSO();
    //delete scaler;
    //scaler = nullptr;
    
  }
}

