

Texture2D yuvtex : register(t0);
Texture2D tex1 : register(t1);

SamplerState samp : register(s0);
cbuffer PS_CONSTANTS : register(b0)
{
    float4 size0;
    float inputPtY;
    float inputPtX;
    float2 fArgs;
    int4 iArgs;
    float2 int_argsf;
    int2 int_argsi;
};

const static float3x3 _rgb2yuv =
{
    0.299, 0.587, 0.114,
	-0.169, -0.331, 0.5,
	0.5, -0.419, -0.081
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD;
};

const static float3x3 _yuv2rgb =
{
    1, -0.00093, 1.401687,
	1, -0.3437, -0.71417,
	1, 1.77216, 0.00099
};

float4 main(PS_INPUT input) : SV_Target
{
    float2 f = frac(input.Tex / float2(inputPtX, inputPtY));
    int2 i = int2(f * 2);
    int index = i.x * 2 + i.y;
    float2 pos1 = input.Tex + (float2(0.5, 0.5) - f) * float2(inputPtX, inputPtY);

    float luma = tex1.Sample(samp, pos1)[index];
    float3 yuv = yuvtex.Sample(samp, input.Tex).xyz;

    return float4(mul(_yuv2rgb, float3(luma, yuv.yz) - float3(0, 0.5, 0.5)), 1);
}