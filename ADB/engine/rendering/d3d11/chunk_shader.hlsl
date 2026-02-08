struct VS_INPUT
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};


cbuffer TransformBuffer : register(b0)
{
    float4x4 World;
    float4x4 View;
    float4x4 Projection;
};


PS_INPUT VS(VS_INPUT Input)
{
    PS_INPUT Output;
    
    float4 WorldPos = mul(World, float4(Input.Position, 1.0));
    float4 ViewPos  = mul(View, WorldPos);
    Output.Position = mul(Projection, ViewPos);
    Output.TexCoord = Input.TexCoord;
    
    return Output;
}


Texture2D    AlbedoTexture :  register(t0);
SamplerState TextureSampler : register(s0);


float4 PS(PS_INPUT Input) : SV_TARGET
{
    float3 Albedo = AlbedoTexture.Sample(TextureSampler, Input.TexCoord).rgb;
    return float4(Albedo, 1.f);
}