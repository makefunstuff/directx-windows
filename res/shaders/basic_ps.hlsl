struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

float4 PSMain(PS_INPUT input) : SV_Target
{
    return input.Color;
}
