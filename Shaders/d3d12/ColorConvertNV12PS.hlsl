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

#include "ShaderUtility.hlsli"
#include "PresentRS.hlsli"


Texture2D ColorTex[2] : register(t0);

cbuffer Constants : register(b0)
{
    float3 cm_r;
    float3 cm_g;
    float3 cm_b;
    float3 cm_c;
}

SamplerState BilinearFilter : register(s0);

[RootSignature(Present_RootSig)]
float3 main(float4 position : SV_Position, float2 uv : TexCoord0) : SV_Target0
{
    float colorY;
    float2 colorUV;
    colorY = ColorTex[0].Sample(BilinearFilter, uv).r;
    colorUV = ColorTex[1].Sample(BilinearFilter, uv).rg;
    float3 color = float3(colorY, colorUV);

    color.rgb = float3(mul(cm_r, color), mul(cm_g, color), mul(cm_b, color)) + cm_c;
    return color;
}
