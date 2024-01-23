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

Texture2D t0 : register(t0);
SamplerState s0 : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    float3 lightDirection_viewSpace = normalize((input.lightPosition_viewSpace - input.vertexPosition_viewSpace).xyz);
    float3 diffuseStrength = clamp(dot(input.normal_viewSpace.xyz, lightDirection_viewSpace), 0.0, 1.0);

    return t0.Sample(s0, input.texCoord) * input.color * float4(diffuseStrength, 1);
}