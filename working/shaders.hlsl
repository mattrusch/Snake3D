// shaders.hlsl

cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 mWorldViewProj;
};

cbuffer OffsetBuffer : register(b1)
{
    float4x4 instanceWorldViewProj;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PsInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texcoords : TEXCOORD;
};

PsInput VsMain(float4 position : POSITION, float4 color : COLOR, float2 texcoords : TEXCOORD)
{
    PsInput result;

    result.position = mul(instanceWorldViewProj, position);
    result.color = color;
    result.texcoords = texcoords;

    return result;
}

float4 PsMain(PsInput input) : SV_TARGET
{
    float4 texCol = gTexture.Sample(gSampler, input.texcoords);
    return input.color * texCol;
}
