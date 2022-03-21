#define PresentNv12_RootSig \
    "RootFlags(0), " \
    "DescriptorTable(SRV(t0, numDescriptors = 2))," \
    "RootConstants(b0, num32BitConstants = 6), " \
    "SRV(t2, visibility = SHADER_VISIBILITY_PIXEL)," \
    "DescriptorTable(UAV(u0, numDescriptors = 2)), " \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s1," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_POINT)"

cbuffer PS_CONSTANT_BUFFER : register(b0)
{
    float4x3 Colorspace;
    float Opacity;
    float LuminanceScale;
    float2 Boundary;
};

struct VEC4I
{
    int x;
    int y;
    int z;
    int w;
};

struct v2f
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv : TEXCOORD0;
};

Texture2D<float4> tex : register(t0);
Texture2D<float4> tex2 : register(t1);
SamplerState normalSampler : register(s0);
//SamplerState borderSampler : register(s1);

inline float3 toneMapping(float3 rgb)
{
    rgb = rgb * LuminanceScale;
    return rgb;
}

inline float4 sampleTexture(SamplerState samplerState, float2 coords)
{
    float4 sample;
    /* sampling routine in sample */

    sample.rgb = tex.Sample(samplerState, coords).rgb;
    sample.z = tex2.Sample(samplerState, coords).z;
    
    //shaderTexture1.Sample(samplerState, coords).xy;
    sample.a = 1;

    return sample;
}

[RootSignature(PresentNv12_RootSig)]
float4 main(v2f IN) : SV_TARGET0
{
    int2 dimensions;
    uint2 downsampling;
    int y_channel;
    int u_channel;
    int v_channel;
    int mode;
    dimensions.x = 1280;
    dimensions.y = 528;
    downsampling.x = 2;
    downsampling.y = 0;
    y_channel = 0;
    u_channel = 4;
    v_channel = 5;
    mode = 1;

    uint3 coord = uint3(IN.uv.xy * float2(dimensions.xy), 0);

    bool use_second_y = false;

  // detect interleaved 4:2:2.
  // 4:2:0 will have downsampling.x == downsampling.y == 2,
  // 4:4:4 will have downsampling.x == downsampling.y == 1
  // planar formats will have one one channel >= 4 i.e. in the second texture.
    if (downsampling.x > downsampling.y && y_channel < 4 && u_channel < 4 && v_channel < 4)
    {
    // if we're in an odd pixel, use second Y sample. See below
        use_second_y = ((coord.x & 1u) != 0);
    // downsample co-ordinates
        coord.xy /= downsampling.xy;
    }

    float4 texvec = tex.Load(coord);

  // if we've sampled interleaved YUYV, for odd x co-ords we use .z for luma
    if (use_second_y)
        texvec.x = texvec.z;

    if (mode == 0)
        return texvec;

    coord = uint3(IN.uv.xy * float2(dimensions.xy), 0);

  // downsample co-ordinates for second texture
    //coord.xy /= downsampling.xy;
    coord.x /= downsampling.x;

    float4 texvec2 = tex2.Load(coord);

    float texdata[] =
    {
        texvec.x, texvec.y, texvec.z, texvec.w,
    texvec2.x, texvec2.y, texvec2.z, texvec2.w,
    };

    float Y = texdata[y_channel];
    float U = texdata[u_channel];
    float V = texdata[v_channel];
    float A = float(texvec.w);

    const float Kr = 0.2126f;
    const float Kb = 0.0722f;

    float L = Y;
    float Pb = U - 0.5f;
    float Pr = V - 0.5f;

  // these are just reversals of the equations below

    float B = L + (Pb / 0.5f) * (1 - Kb);
    float R = L + (Pr / 0.5f) * (1 - Kr);
    float G = (L - Kr * R - Kb * B) / (1.0f - Kr - Kb);

    return float4(R, G, B, A);

}
