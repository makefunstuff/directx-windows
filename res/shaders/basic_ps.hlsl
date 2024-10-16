Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
    float2 Tex : TEXCOORD0;
};

float4 PS_Main(PS_INPUT input) : SV_Target
{
    return txDiffuse.Sample(samLinear, input.Tex) * input.Color;
}
