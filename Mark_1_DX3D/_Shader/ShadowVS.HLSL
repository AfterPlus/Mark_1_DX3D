cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    matrix lightViewMatrix;
    matrix lightProjectionMatrix;
};

cbuffer LightPositionBuffer : register(b1)
{
    float3 lightPosition;
    float padding;
};

struct VertexInputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float4 lightViewPosition : TEXCOORD1;
    float3 lightPos : TEXCOORD2;
};

PixelInputType ShadowVertexShader(VertexInputType input)
{
    PixelInputType output;
    float4 worldPosition;

    input.position.w = 1.0f;

    // Position as seen by the camera.
    worldPosition = mul(input.position, worldMatrix);
    output.position = mul(worldPosition, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    // Position as seen by the light, used to look up the shadow map.
    output.lightViewPosition = mul(worldPosition, lightViewMatrix);
    output.lightViewPosition = mul(output.lightViewPosition, lightProjectionMatrix);

    output.tex = input.tex;

    output.normal = normalize(mul(input.normal, (float3x3)worldMatrix));

    // Direction from the vertex to the light.
    output.lightPos = normalize(lightPosition.xyz - worldPosition.xyz);

    return output;
}
