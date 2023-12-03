struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD;
};

Texture2D t0 : register(t0);
SamplerState s0 : register(s0);

float4 PSMain(PSInput input) : SV_TARGET
{
    //return input.color;
    return t0.Sample(s0, input.texCoord) * input.color;
}