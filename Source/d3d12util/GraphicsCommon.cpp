//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#include "stdafx.h"
#include "GraphicsCommon.h"
#include "SamplerManager.h"
#include "CommandSignature.h"
#include "Texture.h"
#include "PipelineState.h"
#include "RootSignature.h"
#include "BufferManager.h"

namespace D3D12Engine
{
    SamplerDesc SamplerLinearWrapDesc;
    SamplerDesc SamplerAnisoWrapDesc;
    SamplerDesc SamplerShadowDesc;
    SamplerDesc SamplerLinearClampDesc;
    SamplerDesc SamplerVolumeWrapDesc;
    SamplerDesc SamplerPointClampDesc;
    SamplerDesc SamplerPointBorderDesc;
    SamplerDesc SamplerLinearBorderDesc;
    SamplerDesc SamplerGeometryDesc;

    D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearWrap;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerAnisoWrap;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerShadow;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearClamp;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerVolumeWrap;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointClamp;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointBorder;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearBorder;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerGeometry;

    D3D12_RASTERIZER_DESC RasterizerDefault;	// Counter-clockwise
    D3D12_RASTERIZER_DESC RasterizerDefaultCw;	// Clockwise winding
    D3D12_RASTERIZER_DESC RasterizerTwoSided;


    D3D12_BLEND_DESC BlendNoColorWrite;
    D3D12_BLEND_DESC BlendDisable;
    D3D12_BLEND_DESC BlendPreMultiplied;
    D3D12_BLEND_DESC BlendGeometry;

    D3D12_DEPTH_STENCIL_DESC DepthStateDisabled;

    CommandSignature DispatchIndirectCommandSignature(1);
    CommandSignature DrawIndirectCommandSignature(1);

    RootSignature g_CommonRS;
    ComputePSO g_GenerateMipsLinearPSO[4] =
    {
        {L"Generate Mips Linear CS"},
        {L"Generate Mips Linear Odd X CS"},
        {L"Generate Mips Linear Odd Y CS"},
        {L"Generate Mips Linear Odd CS"},
    };

    ComputePSO g_GenerateMipsGammaPSO[4] =
    {
        { L"Generate Mips Gamma CS" },
        { L"Generate Mips Gamma Odd X CS" },
        { L"Generate Mips Gamma Odd Y CS" },
        { L"Generate Mips Gamma Odd CS" },
    };
}

void D3D12Engine::InitializeCommonState(void)
{

    SamplerLinearWrapDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    SamplerLinearWrap = SamplerLinearWrapDesc.CreateDescriptor();

    SamplerAnisoWrapDesc.MaxAnisotropy = 4;
    SamplerAnisoWrap = SamplerAnisoWrapDesc.CreateDescriptor();

    SamplerShadowDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    SamplerShadowDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    SamplerShadowDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    SamplerShadow = SamplerShadowDesc.CreateDescriptor();

    SamplerLinearClampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    SamplerLinearClampDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    SamplerLinearClamp = SamplerLinearClampDesc.CreateDescriptor();

    SamplerVolumeWrapDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    SamplerVolumeWrap = SamplerVolumeWrapDesc.CreateDescriptor();

    SamplerPointClampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    SamplerPointClampDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    SamplerPointClamp = SamplerPointClampDesc.CreateDescriptor();

    SamplerLinearBorderDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    SamplerLinearBorderDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
    SamplerLinearBorderDesc.SetBorderColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
    SamplerLinearBorder = SamplerLinearBorderDesc.CreateDescriptor();

    SamplerPointBorderDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    SamplerPointBorderDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
    SamplerPointBorderDesc.SetBorderColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
    SamplerPointBorder = SamplerPointBorderDesc.CreateDescriptor();
    
    SamplerGeometryDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    SamplerGeometryDesc.MipLODBias = 0.f;
    SamplerGeometryDesc.MaxAnisotropy = 0;
    SamplerGeometryDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    //SamplerGeometryDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    SamplerGeometryDesc.MinLOD = 0.f;
    SamplerGeometryDesc.MaxLOD = 0.f;
    //SamplerGeometryDesc.ShaderRegister = 0;
    //SamplerGeometryDesc.RegisterSpace = 0;
    //SamplerGeometryDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    SamplerGeometry = SamplerGeometryDesc.CreateDescriptor();

    // Default rasterizer states
    RasterizerDefault.FillMode = D3D12_FILL_MODE_SOLID;
    RasterizerDefault.CullMode = D3D12_CULL_MODE_BACK;
    RasterizerDefault.FrontCounterClockwise = TRUE;
    RasterizerDefault.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    RasterizerDefault.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    RasterizerDefault.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    RasterizerDefault.DepthClipEnable = TRUE;
    RasterizerDefault.MultisampleEnable = FALSE;
    RasterizerDefault.AntialiasedLineEnable = FALSE;
    RasterizerDefault.ForcedSampleCount = 0;
    RasterizerDefault.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    RasterizerDefaultCw = RasterizerDefault;
    RasterizerDefaultCw.FrontCounterClockwise = FALSE;

    RasterizerTwoSided = RasterizerDefault;
    RasterizerTwoSided.CullMode = D3D12_CULL_MODE_NONE;


    DepthStateDisabled.DepthEnable = FALSE;
    DepthStateDisabled.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    DepthStateDisabled.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    DepthStateDisabled.StencilEnable = FALSE;
    DepthStateDisabled.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    DepthStateDisabled.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    DepthStateDisabled.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    DepthStateDisabled.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    DepthStateDisabled.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    DepthStateDisabled.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    DepthStateDisabled.BackFace = DepthStateDisabled.FrontFace;

    D3D12_BLEND_DESC alphaBlend = {};
    alphaBlend.IndependentBlendEnable = FALSE;
    alphaBlend.RenderTarget[0].BlendEnable = FALSE;
    alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    alphaBlend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    alphaBlend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    alphaBlend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    alphaBlend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;// D3D12_BLEND_INV_SRC_ALPHA;
    alphaBlend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    alphaBlend.RenderTarget[0].RenderTargetWriteMask = 0;
    BlendNoColorWrite = alphaBlend;

    alphaBlend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    BlendDisable = alphaBlend;

    alphaBlend.RenderTarget[0].BlendEnable = TRUE;
    alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    BlendPreMultiplied = alphaBlend;

    BlendGeometry = {};
    BlendGeometry.RenderTarget[0].BlendEnable = TRUE;
    BlendGeometry.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    BlendGeometry.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    BlendGeometry.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    BlendGeometry.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    BlendGeometry.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    BlendGeometry.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    BlendGeometry.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    //might need them for rendering with normal pictures but right now it make it crash on recreate
    //DispatchIndirectCommandSignature[0].Dispatch();
    //DispatchIndirectCommandSignature.Finalize();

    //DrawIndirectCommandSignature[0].Draw();
    //DrawIndirectCommandSignature.Finalize();
}

void D3D12Engine::DestroyCommonState(void)
{
    //DispatchIndirectCommandSignature.Destroy();
    //DrawIndirectCommandSignature.Destroy();
}
