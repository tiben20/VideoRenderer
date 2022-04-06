/*
 * *******  Super XBR Shader  *******
 * Original developed by 2015 Hyllian - sergiogdb@gmail.com
 * Converted to be able to compile in shadermodel higher dans 3.0
 *
 * (C) 2022 Ti-BEN
 *
 * 
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

//#pragma parameter XBR_EDGE_STR "Xbr - Edge Strength p0" 1.0 0.0 5.0 0.2
//#pragma parameter XBR_WEIGHT "Xbr - Filter Weight" 1.0 0.00 1.50 0.01

/*float4 args0 : register(c2);
float4 size0 : register(c3);

#define XBR_EDGE_STR args0[0]
#define XBR_WEIGHT args0[1]

#define input_size (size0.xy)
#define pixel_size (size0.zw)*/


/* COMPATIBILITY
	 - HLSL compilers
	 - Cg   compilers
*/


#define XBR_EDGE_STR args0[0]
#define XBR_WEIGHT args0[1]
#define input_size (size0.xy)
#define pixel_size (size0.zw)

Texture2D tex : register(t0);
SamplerState samp : register(s0);
cbuffer PS_CONSTANTS : register(b0)
{
    float4 args0;
    float4 size0;
    int PASSNUMBER;
    int FAST_METHOD;
};
float GetWeight(int idx, int thepass)
{
	if (thepass == 1 && idx == 1)
        return (XBR_WEIGHT * 1.75068 / 10.0);
    if (thepass == 1 && idx == 2)
        return (XBR_WEIGHT * 1.29633 / 10.0 / 2.0);
    if (thepass != 1 && idx == 1)
        return (XBR_WEIGHT * 1.29633 / 10.0);
    if (thepass != 1 && idx == 2)
        return (XBR_WEIGHT * 1.75068 / 10.0 / 2.0);
    return 0.0f;
}

const static float3 Y = float3(.2126, .7152, .0722);

float RGBtoYUV(float3 color)
{
	return dot(color, Y);
}

float df(float A, float B)
{
    return abs(A - B);
}


struct WP
{
    float wp1, wp2, wp3, wp4, wp5, wp6;
};

float d_wd(WP wp,float b0, float b1, float c0, float c1, float c2, float d0, float d1, float d2, float d3, float e1, float e2, float e3, float f2, float f3)
{
    return (wp.wp1 * (df(c1, c2) + df(c1, c0) + df(e2, e1) + df(e2, e3)) + wp.wp2 * (df(d2, d3) + df(d0, d1)) + wp.wp3 * (df(d1, d3) + df(d0, d2)) + wp.wp4 * df(d1, d2) + wp.wp5 * (df(c0, c2) + df(e1, e3)) + wp.wp6 * (df(b0, b1) + df(f2, f3)));
}

float hv_wd(WP wp, float i1, float i2, float i3, float i4, float e1, float e2, float e3, float e4)
{
    return (wp.wp4 * (df(i1, i2) + df(i3, i4)) + wp.wp1 * (df(i1, e1) + df(i2, e2) + df(i3, e3) + df(i4, e4)) + wp.wp3 * (df(i1, e2) + df(i3, e4) + df(e1, i2) + df(e3, i4)));
}

float3 min4(float3 a, float3 b, float3 c, float3 d)
{
		return min(a, min(b, min(c, d)));
}
float3 max4(float3 a, float3 b, float3 c, float3 d)
{
		return max(a, max(b, max(c, d)));
}
 
struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD;
};

struct out_vertex {
		float4 position : POSITION;
		float4 color    : COLOR;
		float2 texCoord : TEXCOORD0;
};

float3 GetPixel(float2 inputtex, float x, float y,int thepass)
{
    if (thepass == 0)
        return tex.Sample(samp, inputtex + pixel_size * float2(x, y)).xyz;
    else if (thepass == 1)
        return tex.Sample(samp, inputtex + pixel_size * float2((x) + (y) - 1, (y) - (x))).xyz;
    else
        return tex.Sample(samp, inputtex + pixel_size * float2(x, y)).xyz;
}


float4 main(PS_INPUT input) : SV_Target
{
	
    WP wp;
    if (PASSNUMBER == 0)
    {
        wp.wp1 = 1.0;
        wp.wp2 = 0.0;
        wp.wp3 = 0.0;
        wp.wp4 = 2.0;
        wp.wp5 = -1.0;
        wp.wp6 = 0.0;
    }
    else if (PASSNUMBER == 1)
    {
        wp.wp1 = 1.0;
        wp.wp2 = 0.0;
        wp.wp3 = 0.0;
        wp.wp4 = 4.0;
        wp.wp5 = 0.0;
        wp.wp6 = 0.0;
    }
    else
    {
        wp.wp1 = 1.0;
        wp.wp2 = 0.0;
        wp.wp3 = 0.0;
        wp.wp4 = 0.0;
        wp.wp5 = -1.0;
        wp.wp6 = 0.0;
    }

    if (PASSNUMBER == 0)
    {
        //Skip pixels on wrong grid
        if (any(frac(input.Tex * input_size) < (0.5)))
            return tex.Sample(samp, input.Tex);
    }
    else if (PASSNUMBER == 1)
    {
        //Skip pixels on wrong grid
        float2 dir = frac(input.Tex * input_size / 2.0) - (0.5);
        if ((dir.x * dir.y) > 0.0)
            return tex.Sample(samp, input.Tex);
    }

    
    float3 P0 = GetPixel(input.Tex, -1, -1, PASSNUMBER);
    float3 P1 = GetPixel(input.Tex, 2, -1, PASSNUMBER);
    float3 P2 = GetPixel(input.Tex, -1, 2, PASSNUMBER);
    float3 P3 = GetPixel(input.Tex, 2, 2, PASSNUMBER);

    float3 B = GetPixel(input.Tex, 0, -1, PASSNUMBER);
    float3 C = GetPixel(input.Tex, 1, -1, PASSNUMBER);
    float3 D = GetPixel(input.Tex, -1, 0, PASSNUMBER);
    float3 E = GetPixel(input.Tex, 0, 0, PASSNUMBER);
    float3 F = GetPixel(input.Tex, 1, 0, PASSNUMBER);
    float3 G = GetPixel(input.Tex, -1, 1, PASSNUMBER);
    float3 H = GetPixel(input.Tex, 0, 1, PASSNUMBER);
    float3 I = GetPixel(input.Tex, 1, 1, PASSNUMBER);

    float3 F4 = GetPixel(input.Tex, 2, 0, PASSNUMBER);
    float3 I4 = GetPixel(input.Tex, 2, 1, PASSNUMBER);
    float3 H5 = GetPixel(input.Tex, 0, 2, PASSNUMBER);
    float3 I5 = GetPixel(input.Tex, 1, 2, PASSNUMBER);

    float b = RGBtoYUV(B);
    float c = RGBtoYUV(C);
    float d = RGBtoYUV(D);
    float e = RGBtoYUV(E);
    float f = RGBtoYUV(F);
    float g = RGBtoYUV(G);
    float h = RGBtoYUV(H);
    float i = RGBtoYUV(I);

    float i4 = RGBtoYUV(I4);
    float p0 = RGBtoYUV(P0);
    float i5 = RGBtoYUV(I5);
    float p1 = RGBtoYUV(P1);
    float h5 = RGBtoYUV(H5);
    float p2 = RGBtoYUV(P2);
    float f4 = RGBtoYUV(F4);
    float p3 = RGBtoYUV(P3);

/*
								  P1
		 |P0|B |C |P1|         C     F4          |a0|b1|c2|d3|
		 |D |E |F |F4|      B     F     I4       |b0|c1|d2|e3|   |e1|i1|i2|e2|
		 |G |H |I |I4|   P0    E  A  I     P3    |c0|d1|e2|f3|   |e3|i3|i4|e4|
		 |P2|H5|I5|P3|      D     H     I5       |d0|e1|f2|g3|
							   G     H5
								  P2
*/

	/* Calc edgeness in diagonal directions. */
    float d_edge = (d_wd(wp, d, b, g, e, c, p2, h, f, p1, h5, i, f4, i5, i4) - d_wd(wp, c, f4, b, f, i4, p0, e, i, p3, d, h, i5, g, h5));

	/* Calc edgeness in horizontal/vertical directions. */
    float hv_edge = (hv_wd(wp, f, i, e, h, c, i5, b, h5) - hv_wd(wp, e, f, h, i, d, f4, g, i4));

	/* Filter weights. Two taps only. */
    float4 w1 = float4(-GetWeight(1, PASSNUMBER), +0.5,
    +0.5, -GetWeight(1, PASSNUMBER));
    float4 w2 = float4(-GetWeight(2, PASSNUMBER), GetWeight(2, PASSNUMBER) + 0.25, GetWeight(2, PASSNUMBER) + 0.25, -GetWeight(2, PASSNUMBER));

	/* Filtering and normalization in four direction generating four colors. */
    float3 c1 = mul(w1, float4x3(P2, H, F, P1));
    float3 c2 = mul(w1, float4x3(P0, E, I, P3));
    float3 c3 = (mul(w2, float4x3(D, E, F, F4)) + mul(w2, float4x3(G, H, I, I4)));
    float3 c4 = (mul(w2, float4x3(C, F, I, I5)) + mul(w2, float4x3(B, E, H, H5)));
    float limits, edge_strength;
    float3 color;
    if (FAST_METHOD == 0)
    {
        limits = XBR_EDGE_STR + 0.000001;
        edge_strength = smoothstep(0.0, limits, abs(d_edge));
	/* Smoothly blends the two strongest directions (one in diagonal and the other in vert/horiz direction). */
        color = lerp(lerp(c1, c2, step(0.0, d_edge)), lerp(c3, c4, step(0.0, hv_edge)), 1 - edge_strength);
    }
    else 
    {
        limits = XBR_EDGE_STR + 0.000001;
	    edge_strength = smoothstep(-limits, limits, d_edge);
	    /* Smoothly blends the two directions according to edge strength. */
	    color =  lerp(c1, c2, edge_strength);
    }


	/* Anti-ringing code. */
	float3 min_sample = min4( E, F, H, I ) + lerp((P2-H)*(F-P1), (P0-E)*(I-P3), step(0.0, d_edge));
	float3 max_sample = max4( E, F, H, I ) - lerp((P2-H)*(F-P1), (P0-E)*(I-P3), step(0.0, d_edge));
	color = clamp(color, min_sample, max_sample);

	return float4(color, 1.0);
}