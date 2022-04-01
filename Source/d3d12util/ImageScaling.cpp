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

#include "d3d12util/CompiledShaders/ColorConvertNV12PS.h"
#include "d3d12util/CompiledShaders/BilinearUpsamplePS.h"
#include "d3d12util/CompiledShaders/BicubicHorizontalUpsamplePS.h"
#include "d3d12util/CompiledShaders/BicubicVerticalUpsamplePS.h"
#include "d3d12util/CompiledShaders/BicubicUpsampleCS.h"
#include "d3d12util/CompiledShaders/BicubicUpsampleFast16CS.h"
#include "d3d12util/CompiledShaders/BicubicUpsampleFast24CS.h"
#include "d3d12util/CompiledShaders/BicubicUpsampleFast32CS.h"
#include "d3d12util/CompiledShaders/SharpeningUpsamplePS.h"

#include "d3d12util/CompiledShaders/LanczosHorizontalPS.h"
#include "d3d12util/CompiledShaders/LanczosVerticalPS.h"
#include "d3d12util/CompiledShaders/LanczosCS.h"
#include "d3d12util/CompiledShaders/LanczosFast16CS.h"
#include "d3d12util/CompiledShaders/LanczosFast24CS.h"
#include "d3d12util/CompiledShaders/LanczosFast32CS.h"

#include "d3d12util/CompiledShaders/ScreenQuadPresentVS.h"
#include "d3d12util/CompiledShaders/BufferCopyPS.h"
#include "d3d12util/CompiledShaders/PresentSDRPS.h"
#include "d3d12util/CompiledShaders/PresentHDRPS.h"
#include "d3d12util/CompiledShaders/CompositeSDRPS.h"
#include "d3d12util/CompiledShaders/ScaleAndCompositeSDRPS.h"
#include "d3d12util/CompiledShaders/CompositeHDRPS.h"
#include "d3d12util/CompiledShaders/BlendUIHDRPS.h"
#include "d3d12util/CompiledShaders/ScaleAndCompositeHDRPS.h"

using namespace D3D12Engine;

namespace D3D12Engine
{
    extern RootSignature s_PresentRS;
}

namespace ImageScaling
{
  GraphicsPSO ColorConvertNV12PS(L"Image Scaling: Sharpen Upsample PSO");
  /*Scaling PSO*/
    GraphicsPSO SharpeningUpsamplePS(L"Image Scaling: Sharpen Upsample PSO");
    GraphicsPSO BicubicHorizontalUpsamplePS(L"Image Scaling: Bicubic Horizontal Upsample PSO");
    GraphicsPSO BicubicVerticalUpsamplePS(L"Image Scaling: Bicubic Vertical Upsample PSO");
    GraphicsPSO BilinearUpsamplePS(L"Image Scaling: Bilinear Upsample PSO");
    GraphicsPSO LanczosHorizontalPS(L"Image Scaling: Lanczos Horizontal PSO");
    GraphicsPSO LanczosVerticalPS(L"Image Scaling: Lanczos Vertical PSO");

    enum { kDefaultCS, kFast16CS, kFast24CS, kFast32CS, kNumCSModes };
    ComputePSO LanczosCS[kNumCSModes];
    ComputePSO BicubicCS[kNumCSModes];

    NumVar BicubicUpsampleWeight("Graphics/Display/Image Scaling/Bicubic Filter Weight", -0.5f, -1.0f, -0.25f, 0.25f);
    NumVar SharpeningSpread("Graphics/Display/Image Scaling/Sharpness Sample Spread", 1.0f, 0.7f, 2.0f, 0.1f);
    NumVar SharpeningRotation("Graphics/Display/Image Scaling/Sharpness Sample Rotation", 45.0f, 0.0f, 90.0f, 15.0f);
    NumVar SharpeningStrength("Graphics/Display/Image Scaling/Sharpness Strength", 0.10f, 0.0f, 1.0f, 0.01f);
    BoolVar ForcePixelShader("Graphics/Display/Image Scaling/Prefer Pixel Shader", false);
    /*Rendering PSO*/
    RootSignature s_PresentRS;
    RootSignature s_PresentRSColor;
    GraphicsPSO s_BlendUIPSO(L"Core: BlendUI");
    GraphicsPSO s_BlendUIHDRPSO(L"Core: BlendUIHDR");
    GraphicsPSO PresentSDRPS(L"Core: PresentSDR");
    GraphicsPSO PresentHDRPS(L"Core: PresentHDR");
    GraphicsPSO CompositeSDRPS(L"Core: CompositeSDR");
    GraphicsPSO ScaleAndCompositeSDRPS(L"Core: ScaleAndCompositeSDR");
    GraphicsPSO CompositeHDRPS(L"Core: CompositeHDR");
    GraphicsPSO ScaleAndCompositeHDRPS(L"Core: ScaleAndCompositeHDR");

    void ImageScaling::Initialize(DXGI_FORMAT DestFormat)
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

      PresentHDRPS = PresentSDRPS;
      PresentHDRPS.SetPixelShader(g_pPresentHDRPS, sizeof(g_pPresentHDRPS));
      DXGI_FORMAT SwapChainFormats[2] = { DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM };
      PresentHDRPS.SetRenderTargetFormats(2, SwapChainFormats, DXGI_FORMAT_UNKNOWN);
      PresentHDRPS.Finalize();

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
      /*Scaling*/
      BilinearUpsamplePS.SetRootSignature(s_PresentRS);
      BilinearUpsamplePS.SetRasterizerState(RasterizerTwoSided);
      BilinearUpsamplePS.SetBlendState(BlendDisable);
      BilinearUpsamplePS.SetDepthStencilState(DepthStateDisabled);
      BilinearUpsamplePS.SetSampleMask(0xFFFFFFFF);
      BilinearUpsamplePS.SetInputLayout(0, nullptr);
      BilinearUpsamplePS.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
      BilinearUpsamplePS.SetVertexShader(g_pScreenQuadPresentVS, sizeof(g_pScreenQuadPresentVS));
      BilinearUpsamplePS.SetPixelShader(g_pBilinearUpsamplePS, sizeof(g_pBilinearUpsamplePS));
      BilinearUpsamplePS.SetRenderTargetFormat(DestFormat, DXGI_FORMAT_UNKNOWN);
      BilinearUpsamplePS.Finalize();

      BicubicHorizontalUpsamplePS = BilinearUpsamplePS;
      BicubicHorizontalUpsamplePS.SetPixelShader(g_pBicubicHorizontalUpsamplePS, sizeof(g_pBicubicHorizontalUpsamplePS));
      BicubicHorizontalUpsamplePS.SetRenderTargetFormat(g_HorizontalBuffer.GetFormat(), DXGI_FORMAT_UNKNOWN);
      BicubicHorizontalUpsamplePS.Finalize();

      BicubicVerticalUpsamplePS = BilinearUpsamplePS;
      BicubicVerticalUpsamplePS.SetPixelShader(g_pBicubicVerticalUpsamplePS, sizeof(g_pBicubicVerticalUpsamplePS));
      BicubicVerticalUpsamplePS.Finalize();

      BicubicCS[kDefaultCS].SetRootSignature(s_PresentRS);
      BicubicCS[kDefaultCS].SetComputeShader(g_pBicubicUpsampleCS, sizeof(g_pBicubicUpsampleCS));
      BicubicCS[kDefaultCS].Finalize();
      BicubicCS[kFast16CS].SetRootSignature(s_PresentRS);
      BicubicCS[kFast16CS].SetComputeShader(g_pBicubicUpsampleFast16CS, sizeof(g_pBicubicUpsampleFast16CS));
      BicubicCS[kFast16CS].Finalize();
      BicubicCS[kFast24CS].SetRootSignature(s_PresentRS);
      BicubicCS[kFast24CS].SetComputeShader(g_pBicubicUpsampleFast24CS, sizeof(g_pBicubicUpsampleFast24CS));
      BicubicCS[kFast24CS].Finalize();
      BicubicCS[kFast32CS].SetRootSignature(s_PresentRS);
      BicubicCS[kFast32CS].SetComputeShader(g_pBicubicUpsampleFast32CS, sizeof(g_pBicubicUpsampleFast32CS));
      BicubicCS[kFast32CS].Finalize();

      SharpeningUpsamplePS = BilinearUpsamplePS;
      SharpeningUpsamplePS.SetPixelShader(g_pSharpeningUpsamplePS, sizeof(g_pSharpeningUpsamplePS));
      SharpeningUpsamplePS.Finalize();

      LanczosHorizontalPS = BicubicHorizontalUpsamplePS;
      LanczosHorizontalPS.SetPixelShader(g_pLanczosHorizontalPS, sizeof(g_pLanczosHorizontalPS));
      LanczosHorizontalPS.Finalize();

      LanczosVerticalPS = BilinearUpsamplePS;
      LanczosVerticalPS.SetPixelShader(g_pLanczosVerticalPS, sizeof(g_pLanczosVerticalPS));
      LanczosVerticalPS.Finalize();

      LanczosCS[kDefaultCS].SetRootSignature(s_PresentRS);
      LanczosCS[kDefaultCS].SetComputeShader(g_pLanczosCS, sizeof(g_pLanczosCS));
      LanczosCS[kDefaultCS].Finalize();

      LanczosCS[kFast16CS].SetRootSignature(s_PresentRS);
      LanczosCS[kFast16CS].SetComputeShader(g_pLanczosFast16CS, sizeof(g_pLanczosFast16CS));
      LanczosCS[kFast16CS].Finalize();

      LanczosCS[kFast24CS].SetRootSignature(s_PresentRS);
      LanczosCS[kFast24CS].SetComputeShader(g_pLanczosFast24CS, sizeof(g_pLanczosFast24CS));
      LanczosCS[kFast24CS].Finalize();

      LanczosCS[kFast32CS].SetRootSignature(s_PresentRS);
      LanczosCS[kFast32CS].SetComputeShader(g_pLanczosFast32CS, sizeof(g_pLanczosFast32CS));
      LanczosCS[kFast32CS].Finalize();

    }

    void ImageScaling::PreparePresentHDR(GraphicsContext& Context,ColorBuffer& renderTarget, ColorBuffer& videoSource, CRect renderrect)
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

    void ImageScaling::PreparePresentSDR(GraphicsContext& Context,ColorBuffer& renderTarget, ColorBuffer& videoSource,CRect renderrect)
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
        Context.SetConstants(1, 0.7071f / renderTarget.GetWidth(), 0.7071f / renderTarget.GetHeight());
        Context.TransitionResource(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);
        Context.SetRenderTarget(renderTarget.GetRTV());
        Context.SetViewport(renderrect.left, renderrect.top, renderrect.Width(), renderrect.Height());
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

    void BilinearScale(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, CRect destRect)
    {
        Context.SetPipelineState(BilinearUpsamplePS);
        Context.TransitionResource(dest, D3D12_RESOURCE_STATE_RENDER_TARGET);
        Context.TransitionResource(source, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        Context.SetRenderTarget(dest.GetRTV());
        Context.SetViewportAndScissor(0, 0, destRect.Width(), destRect.Height());
        Context.SetDynamicDescriptor(0, 0, source.GetSRV());
        Context.Draw(3);
    }

    void BicubicScale(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, CRect destRect)
    {
        // On Windows it is illegal to have a UAV of a swap chain buffer. In that case we
        // must fall back to the slower, two-pass pixel shader.
        bool bDestinationUAV = dest.GetUAV().ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;

        // Draw or dispatch
        if (bDestinationUAV && !ForcePixelShader)
        {
            ComputeContext& cmpCtx = Context.GetComputeContext();
            cmpCtx.SetRootSignature(s_PresentRS);

            const float scaleX = (float)source.GetWidth() / (float)destRect.Width();
            const float scaleY = (float)source.GetHeight() / (float)destRect.Height();
            cmpCtx.SetConstants(1, scaleX, scaleY, (float)BicubicUpsampleWeight);

            uint32_t tileWidth, tileHeight, shaderMode;

            if (source.GetWidth() * 16 <= destRect.Width() * 13 &&
                source.GetHeight() * 16 <= destRect.Height() * 13)
            {
                tileWidth = 16;
                tileHeight = 16;
                shaderMode = kFast16CS;
            }
            else if (source.GetWidth() * 24 <= destRect.Width() * 21 &&
                source.GetHeight() * 24 <= destRect.Height() * 21)
            {
                tileWidth = 32; // For some reason, occupancy drops with 24x24, reducing perf
                tileHeight = 24;
                shaderMode = kFast24CS;
            }
            else if (source.GetWidth() * 32 <= destRect.Width() * 29 &&
                source.GetHeight() * 32 <= destRect.Height() * 29)
            {
                tileWidth = 32;
                tileHeight = 32;
                shaderMode = kFast32CS;
            }
            else
            {
                tileWidth = 16;
                tileHeight = 16;
                shaderMode = kDefaultCS;
            }

            cmpCtx.SetPipelineState(BicubicCS[shaderMode]);
            cmpCtx.TransitionResource(source, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            cmpCtx.TransitionResource(dest, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            cmpCtx.SetDynamicDescriptor(0, 0, source.GetSRV());
            cmpCtx.SetDynamicDescriptor(3, 0, dest.GetUAV());

            cmpCtx.Dispatch2D(dest.GetWidth(), dest.GetHeight(), tileWidth, tileHeight);
        }
        else
        {
            Context.TransitionResource(g_HorizontalBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
            Context.SetRenderTarget(g_HorizontalBuffer.GetRTV());
            Context.SetViewportAndScissor(0, 0, destRect.Width(), source.GetHeight());
            Context.SetPipelineState(BicubicHorizontalUpsamplePS);
            Context.SetConstants(1, source.GetWidth(), source.GetHeight(), (float)BicubicUpsampleWeight);
            Context.Draw(3);

            Context.TransitionResource(g_HorizontalBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            Context.TransitionResource(dest, D3D12_RESOURCE_STATE_RENDER_TARGET);
            Context.SetRenderTarget(dest.GetRTV());
            Context.SetViewportAndScissor(0, 0, destRect.Width(), destRect.Height());
            Context.SetPipelineState(BicubicVerticalUpsamplePS);
            Context.SetConstants(1, destRect.Width(), source.GetHeight(), (float)BicubicUpsampleWeight);
            Context.SetDynamicDescriptor(0, 0, g_HorizontalBuffer.GetSRV());
            Context.Draw(3);
        }
    }

    void BilinearSharpeningScale(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, CRect destRect)
    {
        Context.SetPipelineState(SharpeningUpsamplePS);
        Context.TransitionResource(dest, D3D12_RESOURCE_STATE_RENDER_TARGET);
        Context.SetRenderTarget(dest.GetRTV());
        Context.SetViewportAndScissor(0, 0, destRect.Width(), destRect.Height());
        float TexelWidth = 1.0f / source.GetWidth();
        float TexelHeight = 1.0f / source.GetHeight();
        float X = Math::Cos((float)SharpeningRotation / 180.0f * 3.14159f) * (float)SharpeningSpread;
        float Y = Math::Sin((float)SharpeningRotation / 180.0f * 3.14159f) * (float)SharpeningSpread;
        const float WA = (float)SharpeningStrength;
        const float WB = 1.0f + 4.0f * WA;
        float Constants[] = { X * TexelWidth, Y * TexelHeight, Y * TexelWidth, -X * TexelHeight, WA, WB };
        Context.SetConstantArray(1, _countof(Constants), Constants);
        Context.Draw(3);
    }

    void LanczosScale(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, CRect destRect)
    {
        // Constants
        const float srcWidth = (float)source.GetWidth();
        const float srcHeight = (float)source.GetHeight();

        // On Windows it is illegal to have a UAV of a swap chain buffer. In that case we
        // must fall back to the slower, two-pass pixel shader.
        bool bDestinationUAV = dest.GetUAV().ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;

        // Draw or dispatch
        if (bDestinationUAV && !ForcePixelShader)
        {
            ComputeContext& cmpCtx = Context.GetComputeContext();
            cmpCtx.SetRootSignature(s_PresentRS);

            const float scaleX = srcWidth / (float)destRect.Width();
            const float scaleY = srcHeight / (float)destRect.Height();
            cmpCtx.SetConstants(1, scaleX, scaleY);

            uint32_t tileWidth, tileHeight, shaderMode;

            if (source.GetWidth() * 16 <= destRect.Width() * 13 &&
                source.GetHeight() * 16 <= destRect.Height() * 13)
            {
                tileWidth = 16;
                tileHeight = 16;
                shaderMode = kFast16CS;
            }
            else if (source.GetWidth() * 24 <= destRect.Width() * 21 &&
                     source.GetHeight() * 24 <= destRect.Height() * 21)
            {
                tileWidth = 32; // For some reason, occupancy drops with 24x24, reducing perf
                tileHeight = 24;
                shaderMode = kFast24CS;
            }
            else if (source.GetWidth() * 32 <= destRect.Width() * 29 &&
                     source.GetHeight() * 32 <= destRect.Height() * 29)
            {
                tileWidth = 32;
                tileHeight = 32;
                shaderMode = kFast32CS;
            }
            else
            {
                tileWidth = 16;
                tileHeight = 16;
                shaderMode = kDefaultCS;
            }

            cmpCtx.SetPipelineState(LanczosCS[shaderMode]);
            cmpCtx.TransitionResource(source, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            cmpCtx.TransitionResource(dest, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            cmpCtx.SetDynamicDescriptor(0, 0, source.GetSRV());
            cmpCtx.SetDynamicDescriptor(3, 0, dest.GetUAV());

            cmpCtx.Dispatch2D(dest.GetWidth(), dest.GetHeight(), tileWidth, tileHeight);
        }
        else
        {
            Context.SetRootSignature(s_PresentRS);
            Context.SetConstants(1, srcWidth, srcHeight);
            Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            Context.TransitionResource(source, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            Context.TransitionResource(dest, D3D12_RESOURCE_STATE_RENDER_TARGET);

            Context.SetDynamicDescriptor(0, 0, source.GetSRV());

            Context.TransitionResource(g_HorizontalBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
            Context.SetPipelineState(LanczosHorizontalPS);
            Context.SetRenderTarget(g_HorizontalBuffer.GetRTV());
            Context.SetViewportAndScissor(0, 0, g_HorizontalBuffer.GetWidth(), source.GetHeight());
            Context.Draw(3);

            Context.TransitionResource(g_HorizontalBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            Context.SetPipelineState(LanczosVerticalPS);
            Context.SetRenderTarget(dest.GetRTV());
            Context.SetViewportAndScissor(0, 0, destRect.Width(), destRect.Height());
            Context.SetDynamicDescriptor(0, 0, g_HorizontalBuffer.GetSRV());
            Context.Draw(3);
        }
    }
}

void ImageScaling::FreeImageScaling()
{
  /*We need to reset them to 0 if we dont the root signature is not the same when we start a new video*/
  s_PresentRS.Free();
  s_PresentRSColor.Free();
  ColorConvertNV12PS.FreePSO();
  SharpeningUpsamplePS.FreePSO();
  BicubicHorizontalUpsamplePS.FreePSO();
  BicubicVerticalUpsamplePS.FreePSO();
  BilinearUpsamplePS.FreePSO();
  LanczosHorizontalPS.FreePSO();
  LanczosVerticalPS.FreePSO();
  for (int i = 0; i < kNumCSModes; i++)
  {
    LanczosCS[i].FreePSO();
    BicubicCS[i].FreePSO();
  }
  //not sure if we need to touch those
  /*NumVar BicubicUpsampleWeight("Graphics/Display/Image Scaling/Bicubic Filter Weight", -0.5f, -1.0f, -0.25f, 0.25f);
  NumVar SharpeningSpread("Graphics/Display/Image Scaling/Sharpness Sample Spread", 1.0f, 0.7f, 2.0f, 0.1f);
  NumVar SharpeningRotation("Graphics/Display/Image Scaling/Sharpness Sample Rotation", 45.0f, 0.0f, 90.0f, 15.0f);
  NumVar SharpeningStrength("Graphics/Display/Image Scaling/Sharpness Strength", 0.10f, 0.0f, 1.0f, 0.01f);
  BoolVar ForcePixelShader("Graphics/Display/Image Scaling/Prefer Pixel Shader", false);*/
  s_BlendUIPSO.FreePSO();
  s_BlendUIHDRPSO.FreePSO();
  PresentSDRPS.FreePSO();
  PresentHDRPS.FreePSO();
  CompositeSDRPS.FreePSO();
  ScaleAndCompositeSDRPS.FreePSO();
  CompositeHDRPS.FreePSO();
  ScaleAndCompositeHDRPS.FreePSO();
}



void ImageScaling::SetPipelineBilinear(GraphicsContext& Context)
{
  Context.SetPipelineState(BilinearUpsamplePS);
}

void ImageScaling::Upscale(GraphicsContext& Context, ColorBuffer& dest, ColorBuffer& source, eScalingFilter tech, CRect destRect)
{
    //ScopedTimer _prof(L"Image Upscale", Context);

    Context.SetRootSignature(s_PresentRS);
    Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Context.TransitionResource(source, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Context.SetDynamicDescriptor(0, 0, source.GetSRV());

    switch (tech)
    {
    case kBicubic: return BicubicScale(Context, dest, source, destRect);
    case kSharpening: return BilinearSharpeningScale(Context, dest, source, destRect);
    case kBilinear: return BilinearScale(Context, dest, source, destRect);
    case kLanczos: return LanczosScale(Context, dest, source, destRect);
    }
}
