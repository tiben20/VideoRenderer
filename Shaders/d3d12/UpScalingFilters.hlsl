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
//0 nearest not passing through this PS
//1 mitchel
//2 catmull
//3 lanczos2
//3 lanczos3
cbuffer PS_CONSTANTS : register(b0)
{
    float2 wh;
    float2 dxdy;
    float2 scale;
    int filter;
    int axis;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD;
};

#define PI acos(-1.)

float4 spline(PS_INPUT input)
{
    float pos = input.Tex[axis] * wh[axis] - 0.5;
    float t = frac(pos); // calculate the difference between the output pixel and the original surrounding two pixels
    pos = pos - t;
    float4 Q0, Q1, Q2, Q3;
    Q0 = float4(0.0f, 0.0f, 0.0f, 0.0f);
    Q1 = Q2 = Q3 = Q0;
    if (axis == 0)
    {
    // original pixels
        Q0 = tex.Sample(samp, float2((pos - 0.5) * dxdy.x, input.Tex.y));
        Q1 = tex.Sample(samp, float2((pos + 0.5) * dxdy.x, input.Tex.y));
        Q2 = tex.Sample(samp, float2((pos + 1.5) * dxdy.x, input.Tex.y));
        Q3 = tex.Sample(samp, float2((pos + 2.5) * dxdy.x, input.Tex.y));
    }
    else if (axis == 1)
    {
        // original pixels
        Q0 = tex.Sample(samp, float2(input.Tex.x, (pos - 0.5) * dxdy.y));
        Q1 = tex.Sample(samp, float2(input.Tex.x, (pos + 0.5) * dxdy.y));
        Q2 = tex.Sample(samp, float2(input.Tex.x, (pos + 1.5) * dxdy.y));
        Q3 = tex.Sample(samp, float2(input.Tex.x, (pos + 2.5) * dxdy.y));
    }

    // calculate weights
    float t2 = t * t;
    float t3 = t * t2;
    float4 w0123;
    w0123 = float4(0.0f, 0.0f, 0.0f, 0.0f);
    if (filter == 1)
    {
     // Mitchell-Netravali spline4
        w0123 = float4(1., 16., 1., 0.) / 18. + float4(-.5, 0., .5, 0.) * t + float4(5., -12., 9., -2.) / 6. * t2 + float4(-7., 21., -21., 7.) / 18. * t3;
    }
    else if (filter == 2) // Catmull-Rom spline4
    {
        w0123 = float4(-.5, 0., .5, 0.) * t + float4(1., -2.5, 2., -.5) * t2 + float4(-.5, 1.5, -1.5, .5) * t3;
        w0123.y += 1.;
    }

    return w0123[0] * Q0 + w0123[1] * Q1 + w0123[2] * Q2 + w0123[3] * Q3; // interpolation output
}

float4 lanczos2(PS_INPUT input)
{
    float pos = input.Tex[axis] * wh[axis] - 0.5;
    float t = frac(pos); // calculate the difference between the output pixel and the original surrounding two pixels
    pos = pos - t;
    float4 Q0, Q1, Q2, Q3;
    Q0 = float4(0.0f, 0.0f, 0.0f, 0.0f);
    Q1 = Q2 = Q3 = Q0;

    if (axis == 0)
    {
        Q1 = tex.Sample(samp, float2((pos + 0.5) * dxdy.x, input.Tex.y)); // nearest original pixel to the left
        if (t)
        {
            // original pixels
            Q0 = tex.Sample(samp, float2((pos - 0.5) * dxdy.x, input.Tex.y));
            Q2 = tex.Sample(samp, float2((pos + 1.5) * dxdy.x, input.Tex.y));
            Q3 = tex.Sample(samp, float2((pos + 2.5) * dxdy.x, input.Tex.y));
        }
    }
    else if (axis == 1)
    {
        Q1 = tex.Sample(samp, float2(input.Tex.x, (pos + 0.5) * dxdy.y));
        if (t)
        {
            // original pixels
            Q0 = tex.Sample(samp, float2(input.Tex.x, (pos - 0.5) * dxdy.y));
            Q2 = tex.Sample(samp, float2(input.Tex.x, (pos + 1.5) * dxdy.y));
            Q3 = tex.Sample(samp, float2(input.Tex.x, (pos + 2.5) * dxdy.y));
        
    
    

            float4 wset = float3(0., 1., 2.).yxyz + float2(t, -t).xxyy;
            float4 w = sin(wset * PI) * sin(wset * PI * .5) / (wset * wset * PI * PI * .5);

            float wc = 1. - dot(1., w); // compensate truncated window factor by bilinear factoring on the two nearest samples
            w.y += wc * (1. - t);
            w.z += wc * t;

            return w.x * Q0 + w.y * Q1 + w.z * Q2 + w.w * Q3; // interpolation output
        }
    }
    return Q1; // case t == 0. is required to return sample Q1, because of a possible division by 0.
}

float4 lanczos3(PS_INPUT input)
{
    float pos = input.Tex[axis] * wh[axis] - 0.5;
    float t = frac(pos); // calculate the difference between the output pixel and the original surrounding two pixels
    pos = pos - t;

    float4 Q0, Q1, Q2, Q3, Q4, Q5;
    Q0 = float4(0.0f, 0.0f, 0.0f, 0.0f);
    Q1 = Q2 = Q3 = Q4 = Q5 = Q0;
    if (t)
    {
        if (axis == 0)
        {
            Q2 = tex.Sample(samp, float2((pos + 0.5) * dxdy.x, input.Tex.y)); // nearest original pixel to the left
            // original pixels
            Q0 = tex.Sample(samp, float2((pos - 1.5) * dxdy.x, input.Tex.y));
            Q1 = tex.Sample(samp, float2((pos - 1.5) * dxdy.x, input.Tex.y));
            Q3 = tex.Sample(samp, float2((pos + 1.5) * dxdy.x, input.Tex.y));
            Q4 = tex.Sample(samp, float2((pos + 2.5) * dxdy.x, input.Tex.y));
            Q5 = tex.Sample(samp, float2((pos + 3.5) * dxdy.x, input.Tex.y));
        }
        else if (axis == 1)
        {
            Q2 = tex.Sample(samp, float2(input.Tex.x, (pos + 0.5) * dxdy.y));
            // original pixels
            Q0 = tex.Sample(samp, float2(input.Tex.x, (pos - 1.5) * dxdy.y));
            Q1 = tex.Sample(samp, float2(input.Tex.x, (pos - 1.5) * dxdy.y));
            Q3 = tex.Sample(samp, float2(input.Tex.x, (pos + 1.5) * dxdy.y));
            Q4 = tex.Sample(samp, float2(input.Tex.x, (pos + 2.5) * dxdy.y));
            Q5 = tex.Sample(samp, float2(input.Tex.x, (pos + 3.5) * dxdy.y));
        }
        float3 wset0 = float3(2., 1., 0.) * PI + t * PI;
        float3 wset1 = float3(1., 2., 3.) * PI - t * PI;
        float3 wset0s = wset0 * .5;
        float3 wset1s = wset1 * .5;
        float3 w0 = sin(wset0) * sin(wset0s) / (wset0 * wset0s);
        float3 w1 = sin(wset1) * sin(wset1s) / (wset1 * wset1s);

        float wc = 1. - dot(1., w0 + w1); // compensate truncated window factor by linear factoring on the two nearest samples
        w0.z += wc * (1. - t);
        w1.x += wc * t;

        return w0.x * Q0 + w0.y * Q1 + w0.z * Q2 + w1.x * Q3 + w1.y * Q4 + w1.z * Q5; // interpolation output
    }
    return Q2; // case t == 0. is required to return sample Q2, because of a possible division by 0.
}

float4 main(PS_INPUT input) : SV_Target
{
    if (filter == 1 || filter == 2)
        return spline(input);
    else if (filter == 3)
        return lanczos2(input);
    else if (filter == 4)
        return lanczos3(input);
    else
        return input.Pos;
    

}
