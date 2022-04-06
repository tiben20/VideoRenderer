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
#include "BufferManager.h"
#include "CommandContext.h"
#include "EngineTuning.h"

#include "CompiledShaders/DownScalingFilters.h"
#include "CompiledShaders/UpscalingFilters.h"
#include "CompiledShaders/ColorConvertNV12PS.h"
#include "CompiledShaders/Superxbr.h"
#include "CompiledShaders/Superxbr3.h"


#include "CompiledShaders/ScreenQuadPresentVS.h"
#include "CompiledShaders/BufferCopyPS.h"
#include "CompiledShaders/PresentSDRPS.h"
#include "CompiledShaders/PresentHDRPS.h"
#include "CompiledShaders/CompositeSDRPS.h"
#include "CompiledShaders/ScaleAndCompositeSDRPS.h"
#include "CompiledShaders/CompositeHDRPS.h"
#include "CompiledShaders/BlendUIHDRPS.h"
#include "CompiledShaders/ScaleAndCompositeHDRPS.h"

//subpic
#include "CompiledShaders/vs_simple.h"
#include "CompiledShaders/ps_simple.h"
using namespace D3D12Engine;

namespace ImageScaling
{
  GraphicsPSO ColorConvertNV12PS(L"Image Scaling: Sharpen Upsample PSO");
  /*Scaling PSO*/
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
  RootSignature s_SubPicRS;
  

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
    s_PresentRSScaling[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
    s_PresentRSScaling[1].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_ALL);
    s_PresentRSScaling[2].InitAsBufferSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
    s_PresentRSScaling[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
    s_PresentRSScaling.InitStaticSampler(0, SamplerLinearClampDesc);
    s_PresentRSScaling.InitStaticSampler(1, SamplerPointClampDesc);
    s_PresentRSScaling.Finalize(L"Present");

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
    SubPicPS.SetRenderTargetFormats(1, &g_OverlayBuffer.GetFormat(), DXGI_FORMAT_UNKNOWN);
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
    VideoRessourceCopyPS.SetRenderTargetFormats(1, &g_OverlayBuffer.GetFormat(), DXGI_FORMAT_UNKNOWN);
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

    
    //Vertex Shader - Pixel Shader linkage error : Signatures between stages are incompatible.The input stage requires Semantic / Index(POSITION, 0) as input, but it is not provided by the output stage.[STATE_CREATION ERROR #666: CREATEGRAPHICSPIPELINESTATE_SHADER_LINKAGE_SEMANTICNAME_NOT_FOUND]
    //Vertex Shader - Pixel Shader linkage error : Signatures between stages are incompatible.The input stage requires Semantic / Index(COLOR, 0) as input, but it is not provided by the output stage.[STATE_CREATION ERROR #666: CREATEGRAPHICSPIPELINESTATE_SHADER_LINKAGE_SEMANTICNAME_NOT_FOUND]
    //Vertex Shader - Pixel Shader linkage error : Signatures between stages are incompatible.Semantic 'TEXCOORD' is defined for mismatched hardware registers between the output stageand input stage.[STATE_CREATION ERROR #660: CREATEGRAPHICSPIPELINESTATE_SHADER_LINKAGE_REGISTERINDEX]
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

  void Downscale(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, int ScalingFilter, CRect srcRect, CRect destRect)
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
    switch (ScalingFilter)
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
    DownScalingConstantBuffer.filter = ScalingFilter;
    Context.SetDynamicConstantBufferView(1, sizeof(CONSTANT_DOWNSCALE_BUFFER), &DownScalingConstantBuffer);

    Context.Draw(3);
  }

  void UpscaleXbr(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, CRect srcRect, CRect destRect)
  {
    Context.SetRootSignature(s_PresentRSScaling);
    Context.SetPipelineState(SuperXbrFiltersPS);
    Context.TransitionResource(dest, D3D12_RESOURCE_STATE_RENDER_TARGET);
    Context.TransitionResource(source, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Context.SetRenderTarget(dest.GetRTV());
    Context.SetViewportAndScissor(destRect.left, destRect.top, destRect.Width(), destRect.Height());
    Context.SetDynamicDescriptor(0, 0, source.GetSRV());

    __declspec(align(16)) struct CONSTANT_DOWNSCALE_BUFFERINT {
      DirectX::XMFLOAT4 arg0;
      DirectX::XMFLOAT4 size0;
      int pass;
      int fastmethod;
    };

    CONSTANT_DOWNSCALE_BUFFERINT bufconst;
    bufconst.pass = 0;
    bufconst.arg0.x = 0.6f;
    bufconst.arg0.y = 0.6f;
    bufconst.arg0.z = 1.0f;
    bufconst.arg0.w = 1.0f;
    bufconst.size0.x = destRect.Width();
    bufconst.size0.y = destRect.Height();
    bufconst.size0.z = 1.0f / destRect.Width();
    bufconst.size0.w = 1.0f / destRect.Height();
    bufconst.fastmethod = 0;
    Context.SetDynamicConstantBufferView(1, sizeof(CONSTANT_DOWNSCALE_BUFFERINT), &bufconst);
    Context.Draw(3);
    bufconst.pass = 1;
    Context.SetDynamicConstantBufferView(1, sizeof(CONSTANT_DOWNSCALE_BUFFERINT), &bufconst);
    Context.Draw(3);
    

  }
  void Upscale(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, int ScalingFilter, CRect srcRect, CRect destRect)
  {
    if (ScalingFilter == 6)
    {
      UpscaleXbr(Context, dest, source, srcRect, destRect);
      return;
    }
    
    
    Context.SetRootSignature(s_PresentRSScaling);
    Context.SetPipelineState(UpScalingFiltersPS);
    Context.TransitionResource(dest, D3D12_RESOURCE_STATE_RENDER_TARGET);
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
    UpScalingConstantBuffer.filter = ScalingFilter;
    Context.SetDynamicConstantBufferView(1, sizeof(CONSTANT_DOWNSCALE_BUFFER), &UpScalingConstantBuffer);
    Context.Draw(3);
  }

  void FreeImageScaling()
  {
    /*We need to reset them to 0 if we dont the root signature is not the same when we start a new video*/
    s_PresentRSColor.Free();
    s_PresentRSScaling.Free();
    s_SubPicRS.Free();
    ColorConvertNV12PS.FreePSO();
    UpScalingFiltersPS.FreePSO();
    SuperXbrFiltersPS.FreePSO();
    DownScalingFiltersPS.FreePSO();
    SubPicPS.FreePSO();
    VideoRessourceCopyPS.FreePSO();
  }
}

