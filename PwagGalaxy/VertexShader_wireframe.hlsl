struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD;
};

struct wvpMatrixValue
{
    float4x4 wvpMatrix;
};
ConstantBuffer<wvpMatrixValue> constantRootDescriptor : register(b1);

PSInput main(float4 position : POSITION, float4 color : COLOR, float2 uv : UV)
{
    PSInput result;

    result.position = mul(position, constantRootDescriptor.wvpMatrix);

    result.color = color;
    result.texCoord = uv;

    return result;
}