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

struct VS_INPUT
{
    uint VertID : SV_VertexID;
    /*float4 position : POSITION;
		float4 color : COLOR;
		float2 texCoord1 : TEXCOORD0;*/
};

struct VS_OUTPUT
{
        float4 position : POSITION;
		//float4 color : COLOR;
		float2 texCoord0 : TEXCOORD;
    };

VS_OUTPUT main( VS_INPUT input)
{
  VS_OUTPUT output;
    output.texCoord0 = float2(uint2(input.VertID, input.VertID << 1) & 2);
    output.position = float4(lerp(float2(-1, 1), float2(1, -1), output.texCoord0), 0, 1);
  // position w is always 1 for video a video have no depth
    //output.position = input.position;
    //output.texCoord1 = input.texCoord1;
    //output.color = input.color;
  return output;
    // Texture coordinates range [0, 2], but only [0, 1] appears on screen.
    //Tex = float2(uint2(VertID, VertID << 1) & 2);
    //Pos = float4(lerp(float2(-1, 1), float2(1, -1), Tex), 0, 1);
}
