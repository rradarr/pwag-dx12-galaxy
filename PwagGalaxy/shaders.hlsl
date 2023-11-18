struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

cbuffer ConstantBuffer : register(b0)
{
    float4 colorMultiplier;
}

cbuffer ConstantBuffer : register(b1)
{
    float4x4 wvpMatrix;
}

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;

    result.position = mul(position, wvpMatrix);
    //result.position = position;
    result.color = color * colorMultiplier;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}