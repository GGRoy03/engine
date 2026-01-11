struct VS_INPUT
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
    float3 Normal   : NORMAL;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float3 Normal   : NORM;
    float3 WorldPos : WLDP;
};

cbuffer TransformBuffer : register(b0)
{
    float4x4 World;
    float4x4 View;
    float4x4 Projection;
    float3   CameraPos;
};


PS_INPUT VS(VS_INPUT Input)
{
    PS_INPUT Output;
    
    float4 WorldPos = mul(World, float4(Input.Position, 1.0));
    float4 ViewPos  = mul(View, WorldPos);
    Output.Position = mul(Projection, ViewPos);
    
    Output.WorldPos = WorldPos.xyz;
    Output.TexCoord = Input.TexCoord;
    Output.Normal   = normalize(mul((float3x3) World, Input.Normal));
    
    return Output;
}


#define MAX_LIGHTS 16

struct light
{
    float3 Position;
    float  Intensity;
    float3 Color;
    float _Padding;
};


cbuffer LightBuffer : register(b1)
{
    light Lights[MAX_LIGHTS];
    uint  LightCount;
};


Texture2D    ColorTexture     : register(t0);
Texture2D    NormalTexture    : register(t1);
Texture2D    RoughnessTexture : register(t2);
SamplerState TextureSampler   : register(s0);


float4 PS(PS_INPUT Input) : SV_TARGET
{
    float3 Albedo     = ColorTexture.Sample(TextureSampler, Input.TexCoord).rgb;
    float3 Normal     = NormalTexture.Sample(TextureSampler, Input.TexCoord).rgb;
    float  Roughness  = RoughnessTexture.Sample(TextureSampler, Input.TexCoord).r;
    float3 FinalColor = Albedo * 0.2f;
    
    for (uint Idx = 0; Idx < LightCount; ++Idx)
    {
        float3 ToLight = normalize(Lights[Idx].Position - Input.WorldPos); // Vector from where we are to the light
        float  NDotL   = max(dot(Input.Normal, ToLight), 0.f);             // How parallel the surface is to the light
        
        FinalColor += Albedo * Lights[Idx].Color * Lights[Idx].Intensity * NDotL;
        
        float3 ViewDir       = normalize(CameraPos - Input.WorldPos);
        float3 HalfDir       = normalize(ToLight + ViewDir);
        float  SpecularPower = (1.0 - Roughness) * 256;
        float  Specular      = pow(max(dot(Normal, HalfDir), 0.0), SpecularPower);
        
        FinalColor += Specular * Lights[Idx].Color * Lights[Idx].Intensity * 0.5;
    }
    
    return float4(FinalColor, 1.f);
}