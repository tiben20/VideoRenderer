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

//[RootSignature(Present_RootSig)]
VS_OUTPUT main( VS_INPUT input)
{
  VS_OUTPUT output;
  // position w is always 1 for video a video have no depth
  output.Pos = float4(input.position, 1);
  output.Tex = float3(input.uv, 0);
  return output;
    // Texture coordinates range [0, 2], but only [0, 1] appears on screen.
    //Tex = float2(uint2(VertID, VertID << 1) & 2);
    //Pos = float4(lerp(float2(-1, 1), float2(1, -1), Tex), 0, 1);
}
