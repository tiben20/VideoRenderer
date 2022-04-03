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




#include "CompiledShaders/ScreenQuadPresentVS.h"
#include "CompiledShaders/BufferCopyPS.h"
#include "CompiledShaders/PresentSDRPS.h"
#include "CompiledShaders/PresentHDRPS.h"
#include "CompiledShaders/CompositeSDRPS.h"
#include "CompiledShaders/ScaleAndCompositeSDRPS.h"
#include "CompiledShaders/CompositeHDRPS.h"
#include "CompiledShaders/BlendUIHDRPS.h"
#include "CompiledShaders/ScaleAndCompositeHDRPS.h"

using namespace D3D12Engine;

namespace ImageScaling
{
  GraphicsPSO ColorConvertNV12PS(L"Image Scaling: Sharpen Upsample PSO");
  /*Scaling PSO*/
    GraphicsPSO UpScalingFiltersPS(L"Image Scaling: Upscaling filters PSO");
    GraphicsPSO DownScalingFiltersPS(L"Image Scaling: DownScaling filters PSO");
    //constant buffer for downsampling
    CONSTANT_DOWNSCALE_BUFFER DownScalingConstantBuffer;;
    CONSTANT_UPSCALE_BUFFER UpScalingConstantBuffer;;

    enum { kDefaultCS, kFast16CS, kFast24CS, kFast32CS, kNumCSModes };
    ComputePSO LanczosCS[kNumCSModes];
    ComputePSO BicubicCS[kNumCSModes];
    /*Rendering PSO*/
    RootSignature s_PresentRS;
    RootSignature s_PresentRSColor;
    RootSignature s_PresentRSScaling;
    GraphicsPSO s_BlendUIPSO(L"Core: BlendUI");
    GraphicsPSO s_BlendUIHDRPSO(L"Core: BlendUIHDR");
    GraphicsPSO PresentSDRPS(L"Core: PresentSDR");
    GraphicsPSO PresentHDRPS(L"Core: PresentHDR");
    GraphicsPSO CompositeSDRPS(L"Core: CompositeSDR");
    GraphicsPSO ScaleAndCompositeSDRPS(L"Core: ScaleAndCompositeSDR");
    GraphicsPSO CompositeHDRPS(L"Core: CompositeHDR");
    GraphicsPSO ScaleAndCompositeHDRPS(L"Core: ScaleAndCompositeHDR");

    void Initialize(DXGI_FORMAT DestFormat)
    {
      /* Rendering and present PSOs*/
      s_PresentRS.Reset(4, 2);
      s_PresentRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
      s_PresentRS[1].InitAsConstants(0, 32, D3D12_SHADER_VISIBILITY_ALL);
      s_PresentRS[2].InitAsBufferSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
      s_PresentRS[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
      s_PresentRS.InitStaticSampler(0, SamplerLinearClampDesc);
      s_PresentRS.InitStaticSampler(1, SamplerPointClampDesc);
      s_PresentRS.Finalize(L"Present");

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

      s_BlendUIPSO.SetRootSignature(s_PresentRS);
      s_BlendUIPSO.SetRasterizerState(RasterizerTwoSided);
      s_BlendUIPSO.SetBlendState(BlendPreMultiplied);
      s_BlendUIPSO.SetDepthStencilState(DepthStateDisabled);
      s_BlendUIPSO.SetSampleMask(0xFFFFFFFF);
      s_BlendUIPSO.SetInputLayout(0, nullptr);
      s_BlendUIPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
      s_BlendUIPSO.SetVertexShader(g_pScreenQuadPresentVS, sizeof(g_pScreenQuadPresentVS));
      s_BlendUIPSO.SetPixelShader(g_pBufferCopyPS, sizeof(g_pBufferCopyPS));
      s_BlendUIPSO.SetRenderTargetFormat(DestFormat, DXGI_FORMAT_UNKNOWN);
      s_BlendUIPSO.Finalize();

      s_BlendUIHDRPSO = s_BlendUIPSO;
      s_BlendUIHDRPSO.SetPixelShader(g_pBlendUIHDRPS, sizeof(g_pBlendUIHDRPS));
      s_BlendUIHDRPSO.Finalize();

#define CreatePSO( ObjName, ShaderByteCode ) \
    ObjName = s_BlendUIPSO; \
    ObjName.SetBlendState( BlendDisable ); \
    ObjName.SetPixelShader(ShaderByteCode, sizeof(ShaderByteCode) ); \
    ObjName.Finalize();

      CreatePSO(PresentSDRPS, g_pPresentSDRPS);
      CreatePSO(CompositeSDRPS, g_pCompositeSDRPS);
      CreatePSO(ScaleAndCompositeSDRPS, g_pScaleAndCompositeSDRPS);
      CreatePSO(CompositeHDRPS, g_pCompositeHDRPS);
      CreatePSO(ScaleAndCompositeHDRPS, g_pScaleAndCompositeHDRPS);
#undef CreatePSO

      PresentHDRPS = PresentSDRPS;
      PresentHDRPS.SetPixelShader(g_pPresentHDRPS, sizeof(g_pPresentHDRPS));
      DXGI_FORMAT SwapChainFormats[2] = { DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM };
      PresentHDRPS.SetRenderTargetFormats(2, SwapChainFormats, DXGI_FORMAT_UNKNOWN);
      PresentHDRPS.Finalize();

      
      

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

      ColorConvertNV12PS.SetRootSignature(s_PresentRSColor);
      //BilinearUpsamplePS2.SetRasterizerState(D3D12Engine::RasterizerDefault);
      ColorConvertNV12PS.SetRasterizerState(D3D12Engine::RasterizerDefault);
      ColorConvertNV12PS.SetBlendState(D3D12Engine::BlendDisable);
      //BilinearUpsamplePS2.SetBlendState(D3D12Engine::BlendDisable);
      ColorConvertNV12PS.SetDepthStencilState(D3D12Engine::DepthStateDisabled);
      ColorConvertNV12PS.SetSampleMask(0xFFFFFFFF);
      ColorConvertNV12PS.SetInputLayout(0, nullptr);
      ColorConvertNV12PS.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
      ColorConvertNV12PS.SetVertexShader(g_pScreenQuadPresentVS, sizeof(g_pScreenQuadPresentVS));
      ColorConvertNV12PS.SetPixelShader(g_pColorConvertNV12PS, sizeof(g_pColorConvertNV12PS));
      ColorConvertNV12PS.SetRenderTargetFormat(DestFormat, DXGI_FORMAT_UNKNOWN);
      ColorConvertNV12PS.Finalize();

    }

    void PreparePresentHDR(GraphicsContext& Context,ColorBuffer& renderTarget, ColorBuffer& videoSource, CRect renderrect)
    {

      bool NeedsScaling = true;// g_NativeWidth != g_DisplayWidth || g_NativeHeight != g_DisplayHeight;

      Context.SetRootSignature(s_PresentRS);
      Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      Context.TransitionResource(videoSource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
      Context.SetDynamicDescriptor(0, 0, videoSource.GetSRV());

      ColorBuffer& Dest = renderTarget;

      // On Windows, prefer scaling and compositing in one step via pixel shader
      Context.SetRootSignature(s_PresentRS);
      Context.TransitionResource(g_OverlayBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        Context.SetDynamicDescriptor(0, 1, g_OverlayBuffer.GetSRV());
        Context.SetPipelineState(NeedsScaling ? ScaleAndCompositeHDRPS : CompositeHDRPS);
        //NumVar g_HDRPaperWhite("Graphics/Display/Paper White (nits)", 200.0f, 100.0f, 500.0f, 50.0f);
        //NumVar g_MaxDisplayLuminance("Graphics/Display/Peak Brightness (nits)", 1000.0f, 500.0f, 10000.0f, 100.0f);
        uint32_t g_NativeWidth = 800;
        uint32_t g_NativeHeight = 600;
      Context.SetConstants(1, (float)/*g_HDRPaperWhite*/50.0f / 10000.0f, (float)/*g_MaxDisplayLuminance*/500.0f,
        0.7071f / g_NativeWidth, 0.7071f / g_NativeHeight);
      Context.TransitionResource(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);
      Context.SetRenderTarget(renderTarget.GetRTV());
      Context.SetViewport(renderrect.left, renderrect.top, renderrect.Width(), renderrect.Height());
      Context.Draw(3);
    }

    void PreparePresentSDR(GraphicsContext& Context,ColorBuffer& renderTarget, ColorBuffer& videoSource,CRect renderrect)
    {

      Context.SetRootSignature(s_PresentRS);
      Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

      // We're going to be reading these buffers to write to the swap chain buffer(s)
      Context.TransitionResource(videoSource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
      Context.SetDynamicDescriptor(0, 0, videoSource.GetSRV());

      bool NeedsScaling = false;// g_NativeWidth != g_DisplayWidth || g_NativeHeight != g_DisplayHeight;

      // On Windows, prefer scaling and compositing in one step via pixel shader
      if (!NeedsScaling)
      {
        Context.TransitionResource(g_OverlayBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        Context.SetDynamicDescriptor(0, 1, g_OverlayBuffer.GetSRV());
        Context.SetPipelineState(ScaleAndCompositeSDRPS);
        
        Context.TransitionResource(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);
        Context.SetRenderTarget(renderTarget.GetRTV());
        
        Context.SetViewportAndScissor(renderrect.left, renderrect.top, renderrect.Width(), renderrect.Height());
        
        Context.Draw(3);
      }
      else
      {
        ColorBuffer& Dest = renderTarget;

        // Scale or Copy
        if (NeedsScaling)
        {
          //ImageScaling::Upscale(Context, Dest, videoSource, eScalingFilter((int)UpsampleFilter));
        }
        else
        {
          Context.SetPipelineState(PresentSDRPS);
          Context.TransitionResource(Dest, D3D12_RESOURCE_STATE_RENDER_TARGET);
          Context.SetRenderTarget(Dest.GetRTV());
          Context.SetViewport(0, 0, renderrect.Width(), renderrect.Height());
          Context.Draw(3);
        }
      }


    }

    

    void ColorAjust(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source0, ColorBuffer& source1, CONSTANT_BUFFER_VAR& colorconstant)
    {
      Context.SetRootSignature(s_PresentRSColor);
      Context.SetPipelineState(ColorConvertNV12PS);
      Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      Context.SetDynamicConstantBufferView(1, sizeof(CONSTANT_BUFFER_VAR), &colorconstant);
      Context.TransitionResource(dest, D3D12_RESOURCE_STATE_RENDER_TARGET);
      Context.TransitionResourceShutUp(source0, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
      Context.TransitionResourceShutUp(source1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
      Context.SetRenderTarget(dest.GetRTV());
      Context.SetViewport(0, 0, dest.GetWidth(), dest.GetHeight());
      Context.SetDynamicDescriptor(0, 0, source0.GetSRV());
      Context.SetDynamicDescriptor(0, 1, source1.GetSRV());
      Context.Draw(3);
    }




void FreeImageScaling()
{
  /*We need to reset them to 0 if we dont the root signature is not the same when we start a new video*/
  s_PresentRS.Free();
  s_PresentRSColor.Free();
  ColorConvertNV12PS.FreePSO();
  UpScalingFiltersPS.FreePSO();
  DownScalingFiltersPS.FreePSO();
  s_BlendUIPSO.FreePSO();
  s_BlendUIHDRPSO.FreePSO();
  PresentSDRPS.FreePSO();
  PresentHDRPS.FreePSO();
  CompositeSDRPS.FreePSO();
  ScaleAndCompositeSDRPS.FreePSO();
  CompositeHDRPS.FreePSO();
  ScaleAndCompositeHDRPS.FreePSO();
}

void Downscale(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, int ScalingFilter,CRect srcRect, CRect destRect)
{
  //the viewport x 0 y 46 width the width and height of the targetvideo
  Context.SetRootSignature(s_PresentRSScaling);
  Context.SetPipelineState(DownScalingFiltersPS);
  Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  Context.TransitionResource(dest, D3D12_RESOURCE_STATE_RENDER_TARGET);
  Context.SetRenderTarget(dest.GetRTV());
  Context.SetViewportAndScissor(destRect.left, destRect.top, destRect.Width(), destRect.Height());
  Context.SetDynamicDescriptor(0, 0, source.GetSRV());

  DownScalingConstantBuffer.wh = { (float)srcRect.Width(),(float)srcRect.Height() };
  DownScalingConstantBuffer.dxdy = {1.0f / srcRect.Width(),1.0f / srcRect.Height()};
  DownScalingConstantBuffer.scale = { (float)srcRect.Width() / destRect.Width(), (float)srcRect.Height() / destRect.Height()};

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

void Upscale(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, int ScalingFilter, CRect srcRect, CRect destRect)
{

  Context.SetRootSignature(s_PresentRSScaling);
  Context.SetPipelineState(UpScalingFiltersPS);
  Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  Context.TransitionResource(dest, D3D12_RESOURCE_STATE_RENDER_TARGET);
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

}