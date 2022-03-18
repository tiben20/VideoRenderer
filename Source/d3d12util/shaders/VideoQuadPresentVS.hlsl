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
// A vertex shader for full-screen effects without a vertex buffer.  The
// intent is to output an over-sized triangle that encompasses the entire
// screen.  By doing so, we avoid rasterization inefficiency that could
// result from drawing two triangles with a shared edge.
//
// Use null input layout
// Draw(3)

#include "PresentRS.hlsli"

struct VS_INPUT
{
float3 position     : POSITION;
float2 uv           : TEXCOORD;
};

struct VS_OUTPUT
{
  float4 Pos : SV_POSITION;    // Upper-left and lower-right coordinates in clip space
  float2 Tex : TEXCOORD0;        // Upper-left and lower-right normalized UVs
};

[RootSignature(Present_RootSig)]
VS_OUTPUT main( VS_INPUT input, uint VertID : SV_VertexID)
{
  VS_OUTPUT output;
  output.Pos = float4(input.position, 1);
  output.Tex = float3(input.uv, 0);
  return output;
    // Texture coordinates range [0, 2], but only [0, 1] appears on screen.
    //Tex = float2(uint2(VertID, VertID << 1) & 2);
    //Pos = float4(lerp(float2(-1, 1), float2(1, -1), Tex), 0, 1);
}
