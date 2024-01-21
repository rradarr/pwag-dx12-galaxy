struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
};

struct wvpMatrixValue
{
    float4x4 wvpMatrix;
};
ConstantBuffer<wvpMatrixValue> constantRootDescriptor : register(b1);

PSInput main(float4 position : POSITION, float4 color : COLOR, float2 uv : UV, float3 normal : NORMAL)
{
    PSInput result;

    result.position = mul(position, constantRootDescriptor.wvpMatrix);

    result.color = color;
    result.texCoord = uv;
    result.normal = normal;

    return result;
}