cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

struct VertexInputType
{
    float4 position : POSITION;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float4 depthPosition : TEXCOORD0;
};

PixelInputType DepthVertexShader(VertexInputType input)
{
    PixelInputType output;

    input.position.w = 1.0f;

    // Transform the vertex into the clip space of the light.
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    // Store the position so the pixel shader can read the depth.
    output.depthPosition = output.position;

    return output;
}
