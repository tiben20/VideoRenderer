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

//jync options
//#pragma parameter JINC2_WINDOW_SINC "Window Sinc Param" 0.44 0.0 1.0 0.01
//#pragma parameter JINC2_SINC "Sinc Param" 0.82 0.0 1.0 0.01
//#pragma parameter JINC2_AR_STRENGTH "Anti-ringing Strength" 0.5 0.0 1.0 0.1
cbuffer PS_CONSTANTS : register(b0)
{
    float2 wh;
    float2 dxdy;
    float2 scale;
    int filter;
    int axis;
    int lanczostype;
    int splinetype;
    float3 jinc2_params;
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
    if (splinetype == 0)
    {
     // Mitchell-Netravali spline4
        w0123 = float4(1., 16., 1., 0.) / 18. + float4(-.5, 0., .5, 0.) * t + float4(5., -12., 9., -2.) / 6. * t2 + float4(-7., 21., -21., 7.) / 18. * t3;
    }
    else if (splinetype == 1) // Catmull-Rom spline4
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


//jync function

#define pinumbers    3.1415926535897932384626433832795
#define min4(a, b, c, d) min(min(a, b), min(c, d))
#define max4(a, b, c, d) max(max(a, b), max(c, d))

float d(float2 pt1, float2 pt2)
{
    float2 v = pt2 - pt1;
    return sqrt(dot(v, v));
}

float4 resampler(float4 x, float wa, float wb) 
{
	return (x == float4(0.0, 0.0, 0.0, 0.0))
		? float4(wa * wb, wa * wb, wa * wb, wa * wb)
		: sin(x * wa) * sin(x * wb) / (x * x);
}

float4 jinc2(PS_INPUT input)
{
    float2 dx = float2(1.0, 0.0);
    float2 dy = float2(0.0, 1.0);

    float2 pc = input.Tex / dxdy;
    float2 tc = floor(pc - float2(0.5, 0.5)) + float2(0.5, 0.5);

    float wa = jinc2_params[0] * pinumbers;
    float wb = jinc2_params[1] * pinumbers;
    float4x4 weights =
    {
        resampler(float4(d(pc, tc - dx - dy), d(pc, tc - dy), d(pc, tc + dx - dy), d(pc, tc + 2.0 * dx - dy)), wa, wb),
		resampler(float4(d(pc, tc - dx), d(pc, tc), d(pc, tc + dx), d(pc, tc + 2.0 * dx)), wa, wb),
		resampler(float4(d(pc, tc - dx + dy), d(pc, tc + dy), d(pc, tc + dx + dy), d(pc, tc + 2.0 * dx + dy)), wa, wb),
		resampler(float4(d(pc, tc - dx + 2.0 * dy), d(pc, tc + 2.0 * dy), d(pc, tc + dx + 2.0 * dy), d(pc, tc + 2.0 * dx + 2.0 * dy)), wa, wb)
    };

    dx *= dxdy;
    dy *= dxdy;
    tc *= dxdy;

	// reading the texels
	// [ c00, c10, c20, c30 ]
	// [ c01, c11, c21, c31 ]
	// [ c02, c12, c22, c32 ]
	// [ c03, c13, c23, c33 ]
    float3 c00 = tex.Sample(samp, tc - dx - dy).rgb;
    float3 c10 = tex.Sample(samp, tc - dy).rgb;
    float3 c20 = tex.Sample(samp, tc + dx - dy).rgb;
    float3 c30 = tex.Sample(samp, tc + 2.0 * dx - dy).rgb;
    float3 c01 = tex.Sample(samp, tc - dx).rgb;
    float3 c11 = tex.Sample(samp, tc).rgb;
    float3 c21 = tex.Sample(samp, tc + dx).rgb;
    float3 c31 = tex.Sample(samp, tc + 2.0 * dx).rgb;
    float3 c02 = tex.Sample(samp, tc - dx + dy).rgb;
    float3 c12 = tex.Sample(samp, tc + dy).rgb;
    float3 c22 = tex.Sample(samp, tc + dx + dy).rgb;
    float3 c32 = tex.Sample(samp, tc + 2.0 * dx + dy).rgb;
    float3 c03 = tex.Sample(samp, tc - dx + 2.0 * dy).rgb;
    float3 c13 = tex.Sample(samp, tc + 2.0 * dy).rgb;
    float3 c23 = tex.Sample(samp, tc + dx + 2.0 * dy).rgb;
    float3 c33 = tex.Sample(samp, tc + 2.0 * dx + 2.0 * dy).rgb;


    float3 color = mul(weights[0], float4x3(c00, c10, c20, c30));
    color += mul(weights[1], float4x3(c01, c11, c21, c31));
    color += mul(weights[2], float4x3(c02, c12, c22, c32));
    color += mul(weights[3], float4x3(c03, c13, c23, c33));
    color /= dot(mul(weights, float4(1, 1, 1, 1)), 1);

	// Get min/max samples
    float3 min_sample = min4(c11, c21, c12, c22);
    float3 max_sample = max4(c11, c21, c12, c22);
    color = lerp(color, clamp(color, min_sample, max_sample), jinc2_params[2]);

	// final sum and weight normalization
    return float4(color.rgb, 1);
}

float4 main(PS_INPUT input) : SV_Target
{
    //spline mitchell and spline
    if (filter == 4)
        return spline(input);
    else if (filter == 3)
    {
        if (lanczostype == 2)
            return lanczos2(input);
        else if (lanczostype ==3 )
            return lanczos3(input);
    }
    else if (filter == 5)
    {
        //jinc
        return jinc2(input);
    }
        
    
    return input.Pos;
}


