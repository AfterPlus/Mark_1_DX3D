Texture2D shaderTexture : register(t0);
Texture2D shadowTexture : register(t1);
SamplerState SampleTypeClamp : register(s0);
SamplerState SampleTypeWrap : register(s1);

cbuffer LightBuffer : register(b0)
{
    float4 ambientColor;
    float4 diffuseColor;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 lightPos : TEXCOORD1;
    float3 tint : TEXCOORD2;
    float height : TEXCOORD3;
    float4 rootPosition : TEXCOORD4;
};

float4 FoliagePixelShader(PixelInputType input) : SV_TARGET
{
    float4 textureColor, color;
    float2 projectTexCoord;
    float lightIntensity, alpha, shadowValue;

    textureColor = shaderTexture.Sample(SampleTypeWrap, input.tex);

    // Cut-out. With MSAA on, alpha-to-coverage turns alpha into the share of samples covered; sharpening it to about
    // one pixel of falloff keeps the blade edges crisp but smooth at every distance.
    alpha = (textureColor.a - 0.5f) / max(fwidth(textureColor.a), 0.0001f) + 0.5f;
    clip(alpha - 0.01f);

    // Same light as the rest of the scene: ambient + diffuse from the shadow light.
    lightIntensity = saturate(dot(normalize(input.normal), input.lightPos));

    // The black-and-white shadow pass at the root, so a tuft is as dark as the ground it stands on.
    projectTexCoord.x =  input.rootPosition.x / input.rootPosition.w / 2.0f + 0.5f;
    projectTexCoord.y = -input.rootPosition.y / input.rootPosition.w / 2.0f + 0.5f;
    shadowValue = shadowTexture.Sample(SampleTypeClamp, projectTexCoord).r;

    // Same formula as SoftShadowPS, so grass and ground match.
    color = saturate(ambientColor + diffuseColor * lightIntensity) * shadowValue;

    // Darker at the root (the blades shade each other), a touch brighter at the tips.
    color.rgb *= textureColor.rgb * input.tint * lerp(0.55f, 1.1f, saturate(input.height));
    color.a = saturate(alpha);

    return color;
}
