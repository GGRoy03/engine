struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Color    : COLOR;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 Color    : COLOR;
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
    Output.Color    = Input.Color;
    
    return Output;
}


float4 PS(PS_INPUT Input) : SV_TARGET
{   
    return float4(Input.Color, 1.f);
}