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


Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer PS_CONSTANTS : register(b0)
{
    float2 wh;
    float2 dxdy;
    float2 scale;
    float support;
    float ss;
    int filter;
    int axis;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD;
};

#define PI acos(-1.)

inline float boxfilter(float x)
{
    if (x >= -0.5 && x < 0.5)
        return 1.0;
    return 0.0;
}

inline float bilinearfilter(float x)
{
    if (x < 0.0)
        x = -x;
    if (x < 1.0)
        return 1.0 - x;
    return 0.0;
}

inline float hammingfilter(float x)
{
    if (x < 0.0)
        x = -x;
    if (x == 0.0)
        return 1.0;
    if (x >= 1.0)
        return 0.0;
    x *= PI;
    return sin(x) / x * (0.54 + 0.46 * cos(x));
}

#define BI_A -0.5

inline float bicubicfilter(float x)
{
    if (x < 0.0)
        x = -x;
    if (x < 1.0)
        return ((BI_A + 2.0) * x - (BI_A + 3.0)) * x * x + 1;
    if (x < 2.0)
        return (((x - 5) * x + 8) * x - 4) * BI_A;
    return 0.0;
}

inline float sinc_lanczosfilter(float x)
{
    if (x == 0.0)
        return 1.0;
    x *= PI;
    return sin(x) / x;
}

inline float lanczosfilter(float x)
{
    /* truncated sinc */
    if (-3.0 <= x && x < 3.0)
        return sinc_lanczosfilter(x) * sinc_lanczosfilter(x / 3);
    return 0.0;
}

//static const float support = filter_support * scale[AXIS];
//static const float ss = 1.0 / scale[AXIS];

#pragma warning(disable: 3595) // disable warning X3595: gradient instruction used in a loop with varying iteration; partial derivatives may have undefined value


float4 main(PS_INPUT input) : SV_Target
{
    float pos = input.Tex[axis] * wh[axis] + 0.5;

    int low = (int) floor(pos - support);
    int high = (int) ceil(pos + support);

    float ww = 0.0;
    float4 avg = 0;
    
//0 box 
//1 bilinear
//2 hamming
//3 bicubic
//4 lanczos
    
    [loop]
    for (int n = low; n < high; n++)
    {
        float w;
        //= filter((n - pos + 0.5) * ss);
        if (filter == 0)
            w = boxfilter((n - pos + 0.5) * ss);
        else if (filter == 1)
            w = bilinearfilter((n - pos + 0.5) * ss);
        else if (filter == 2)
            w = hammingfilter((n - pos + 0.5) * ss);
        else if (filter == 3)
            w = bicubicfilter((n - pos + 0.5) * ss);
        else if (filter == 4)
            w = lanczosfilter((n - pos + 0.5) * ss);
        
        ww += w;
        if (axis == 0)
            avg += w * tex.Sample(samp, float2((n + 0.5) * dxdy.x, input.Tex.y));
        else
            avg += w * tex.Sample(samp, float2(input.Tex.x, (n + 0.5) * dxdy.y));
    }
    avg /= ww;

    return avg;
}
