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

#include "ShaderUtility.hlsli"
#include "PresentRS.hlsli"

//static const float2 wh = { 720, 404 };
//static const float2 dxdy2 = { 2.0 / 720, 2.0 / 404 };

Texture2D ColorTex[2] : register(t0);

cbuffer Constants : register(b0)
{
    float3 cm_r;
    float3 cm_g;
    float3 cm_b;
    float3 cm_c;
}

SamplerState BilinearFilter : register(s0);
float3 reverse_color(float3 floatin)
{
    float3 floatout;
    floatout.r = floatin.g;
    floatout.b = floatin.b;
    floatout.g = floatin.r;
    return floatout;
}
[RootSignature(Present_RootSig)]
float3 main(float4 position : SV_Position, float2 uv : TexCoord0) : SV_Target0
{
    float4 LinearRGB;
    //LinearRGB = ColorTex[0].Sample(BilinearFilter, uv);
    //LinearRGB.gb = ColorTex[1].Sample(BilinearFilter, uv).gb;
    float colorY;
    float2 colorUV;
    colorY = ColorTex[0].Sample(BilinearFilter, uv).r;
    colorUV = ColorTex[1].Sample(BilinearFilter, uv).rg;
    float3 color = float3(colorY, colorUV);
    //float3 cm_r, cm_g, cm_b, cm_c;
    //cm_r.x = 1.16438353;
    //cm_r.y = 0.000000000;
    //cm_r.z = 1.59602678;
    //cm_r.w = 0.000000000;
    //cm_g.x = 1.16438353;
    //cm_g.y = -0.391762257;
    //cm_g.z = -0.812967598;
    //cm_g.w = 0.000000000;
    //cm_b.x = 1.16438353;
    //cm_b.y = 2.01723218;
    //cm_b.z = 0.000000000;
    //cm_b.w = 0.000000000;
    //cm_c.x = -0.874202192;
    //cm_c.xy = 0.531667769;
    //cm_c.xz = -1.08563077;
    //cm_c.xw = 0.000000000;

    color.rgb = float3(mul(cm_r, color), mul(cm_g, color), mul(cm_b, color)) + cm_c;
    return ApplyREC709Curve(color);
    //LinearRGB.yz = ColorTex[0].Sample(BilinearFilter, uv).yz;
   
    //float3 LinearRGB = ColorTex.SampleLevel(BilinearFilter, uv, 0);
    //float3 LinearRGB = RemoveDisplayProfile(ColorTex.SampleLevel(BilinearFilter, uv, 0), LDR_COLOR_FORMAT);
    return LinearRGB;
    //return ApplyDisplayProfile(LinearRGB, DISPLAY_PLANE_FORMAT);
}
