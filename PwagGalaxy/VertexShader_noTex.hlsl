struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD;
};

struct ColorMultiplierValue
{
    float4 colorMultiplier;
};
ConstantBuffer<ColorMultiplierValue> constantDescriptorTable : register(b0);

struct wvpMatrixValue
{
    float4x4 wvpMatrix;
};
ConstantBuffer<wvpMatrixValue> constantRootDescriptor : register(b1);

//cbuffer ConstantBuffer : register(b0)
//{
//    float4 colorMultiplier;
//}

//cbuffer ConstantBuffer : register(b1)
//{
//    float4x4 wvpMatrix;
//}

PSInput main(float4 position : POSITION, float4 color : COLOR, float2 uv : UV)
{
    PSInput result;

    result.position = mul(position, constantRootDescriptor.wvpMatrix);
    //result.position = position;
    result.color = color * constantDescriptorTable.colorMultiplier;
    result.texCoord = uv;

    return result;
}