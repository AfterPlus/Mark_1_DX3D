struct PixelInputType
{
    float4 position : SV_POSITION;
    float4 depthPosition : TEXCOORD0;
};

float4 DepthPixelShader(PixelInputType input) : SV_TARGET
{
    float depthValue;

    // Depth is the z value divided by the homogeneous w value.
    depthValue = input.depthPosition.z / input.depthPosition.w;

    return float4(depthValue, depthValue, depthValue, 1.0f);
}
