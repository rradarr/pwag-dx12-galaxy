struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float4 lightPosition_viewSpace : LIGHT;
    float4 vertexPosition_viewSpace : VERTVIEW;
    float3 normal_viewSpace : NORMVIEW;
};

struct wvpMatrixValue
{
    float4x4 wvpMatrix;
    float4x4 worldMatrix;
    float4x4 viewMatrix;
    float4x4 projMatrix;
};
ConstantBuffer<wvpMatrixValue> constantRootDescriptor : register(b0);

struct lightParams
{
    float3 lightPosition;
};
ConstantBuffer<lightParams> lightConstants : register(b1);


PSInput main(float4 position : POSITION, float4 color : COLOR, float2 uv : UV, float3 normal : NORMAL)
{
    PSInput result;

    // Calculate components for light calculations.
    float4 vertexPosition_worldSpace = mul(position, constantRootDescriptor.worldMatrix);
    float4 vertexPosition_viewSpace = mul(vertexPosition_worldSpace, constantRootDescriptor.viewMatrix);
    result.vertexPosition_viewSpace = vertexPosition_viewSpace;
    result.position = mul(vertexPosition_viewSpace, constantRootDescriptor.projMatrix);

    float4 normal_worldSpace = normalize(mul(float4(normal, 0), constantRootDescriptor.worldMatrix));
    float3 normal_viewSpace = normalize(mul(normal_worldSpace, constantRootDescriptor.viewMatrix));
    result.normal_viewSpace = normal_viewSpace;

    float4 lightPosition_viewSpace = mul(float4(lightConstants.lightPosition, 1), constantRootDescriptor.viewMatrix);
    result.lightPosition_viewSpace = lightPosition_viewSpace;

    result.color = color;
    result.texCoord = uv;
    result.normal = normal;

    return result;
}