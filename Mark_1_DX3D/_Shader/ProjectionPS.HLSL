Texture2D shaderTexture : register(t0);
Texture2D projectionTexture : register(t1);
SamplerState SampleType : register(s0);

cbuffer LightBuffer : register(b0)
{
    float4 ambientColor;
    float4 diffuseColor;
    float lightBrightness;
    float3 padding;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float4 viewPosition : TEXCOORD1;
    float3 lightPos : TEXCOORD2;
};

float4 ProjectionPixelShader(PixelInputType input) : SV_TARGET
{
    float4 color;
    float4 textureColor;
    float lightIntensity;
    float2 projectTexCoord;
    float4 projectionColor;

    // Set the default output color to the ambient light value for all pixels.
    color = ambientColor;

    // Calculate the amount of light on this pixel from the point light.
    lightIntensity = saturate(dot(input.normal, input.lightPos));

    // Sample the surface texture.
    textureColor = shaderTexture.Sample(SampleType, input.tex);

    // Calculate the projected texture coordinates.
    projectTexCoord.x =  input.viewPosition.x / input.viewPosition.w / 2.0f + 0.5f;
    projectTexCoord.y = -input.viewPosition.y / input.viewPosition.w / 2.0f + 0.5f;

    // Pixels inside the projected view port are lit by the point light, scaled by the light map.
    if((saturate(projectTexCoord.x) == projectTexCoord.x) && (saturate(projectTexCoord.y) == projectTexCoord.y))
    {
        // The light map is black and white so only one channel is needed.
        projectionColor = projectionTexture.Sample(SampleType, projectTexCoord);

        if((lightIntensity > 0.0f) && (projectionColor.r > 0.0f))
        {
            color += (diffuseColor * lightIntensity * lightBrightness * projectionColor.r);
        }
    }

    // Saturate the final light color and combine it with the surface texture.
    color = saturate(color);
    color = color * textureColor;

    return color;
}
