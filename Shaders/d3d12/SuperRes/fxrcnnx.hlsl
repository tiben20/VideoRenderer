

Texture2D tex : register(t0);
//Texture2D featureMap1 : register(t0);
Texture2D featureMap2 : register(t1);
Texture2D tex1 : register(t2);
Texture2D tex2 : register(t3);
Texture2D tex3 : register(t4);
Texture2D tex4 : register(t5);
SamplerState samp : register(s0);
float inputPtY;
float inputPtX;
Texture2D yuvTex;

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

float4 main(PS_INPUT input) : SV_Target
{
    float3 color = tex.Sample(samp, input.Tex).rgb;
    return float4(mul(_rgb2yuv, color) + float3(0, 0.5, 0.5), 1);
}