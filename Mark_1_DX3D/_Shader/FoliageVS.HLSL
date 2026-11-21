// Instanced grass. Every model is 1 unit tall (root at y = 0, tip at y = 1); each instance places, turns, sizes and
// tints one copy, and the wind bends it more the higher up the blade a vertex is.

cbuffer FrameBuffer : register(b0)
{
    matrix viewMatrix;
    matrix projectionMatrix;
    float3 lightPosition;
    float time;
    float3 cameraPosition;
    float windStrength;     // how far the tips lean (in blade heights)
    float2 windDirection;   // XZ, normalised
    float windSpeed;        // wave speed
    float fadeStart;        // metres from the camera where the grass starts to shrink away
    float fadeEnd;          // ... and where it is gone
    float lodMaxWiden;      // how much wider the kept far instances may get
    float2 padding;
};

cbuffer ModelBuffer : register(b1)
{
    float lodDistance;      // all instances of this model are drawn up to here, then they thin out ...
    float lodMinKeep;       // ... but never below this share
    float2 modelPadding;
};

struct VertexInputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;

    // Per instance.
    float4 rootYaw : TEXCOORD1;     // xyz = root on the ground, w = rotation around Y (radians)
    float4 shape : TEXCOORD2;       // x = height, y = width scale, z = wind phase, w = stiffness (0 = rigid, 1 = soft)
    float4 tintRank : TEXCOORD3;    // rgb = colour multiplier, a = LOD rank (0..1)
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 lightPos : TEXCOORD1;
    float3 tint : TEXCOORD2;
    float height : TEXCOORD3;       // 0 at the root, 1 at the tip
    float4 rootPosition : TEXCOORD4;    // the root on screen (clip space): where the pixel shader looks up the shadow
};

PixelInputType FoliageVertexShader(VertexInputType input)
{
    PixelInputType output;
    float3 root, local, worldPosition, normal, offset;
    float2 perpendicular;
    float s, c, height, distance, fade, keep, widen, along, gust, wave, flutter, lean, bend, size, width;

    root = input.rootYaw.xyz;
    height = input.position.y;

    distance = length(root.xz - cameraPosition.xz);

    // Shrink the far grass to nothing instead of letting it pop in at the end of the placed area.
    fade = saturate((fadeEnd - distance) / max(fadeEnd - fadeStart, 0.001f));

    // Level of detail (same formula as FoliageKeepShare on the CPU): further away only the instances with a low rank
    // are kept, and those get wider so the field still looks full. Instances near the cut shrink out smoothly.
    keep = 1.0f;
    if(distance > lodDistance)
    {
        keep = max((lodDistance / distance) * (lodDistance / distance), lodMinKeep);
    }

    fade *= saturate((keep - input.tintRank.a) / max(keep * 0.3f, 0.001f));
    widen = min(rsqrt(keep), lodMaxWiden);

    size = input.shape.x * fade;
    width = input.shape.y * widen;

    // Size, then rotation around Y (same as XMMatrixRotationY).
    local = float3(input.position.x * width, input.position.y, input.position.z * width) * size;

    sincos(input.rootYaw.w, s, c);
    local = float3(local.x * c + local.z * s, local.y, -local.x * s + local.z * c);
    normal = float3(input.normal.x * c + input.normal.z * s, input.normal.y, -input.normal.x * s + input.normal.z * c);

    // Wind: slow gusts roll across the field along the wind direction, a faster sway on top, and a little flutter
    // sideways. The root never moves; the bend grows with the square of the height so the blade curves.
    along = dot(root.xz, windDirection);
    gust = sin(time * 0.55f - along * 0.045f) * 0.5f + 0.5f;
    gust = gust * gust;
    wave = sin(time * windSpeed - along * 0.35f + input.shape.z) * 0.65f +
           sin(time * windSpeed * 2.3f - along * 0.8f + input.shape.z * 1.7f) * 0.35f;
    flutter = sin(time * 6.5f + input.shape.z * 3.0f + height * 4.0f) * 0.05f;

    lean = windStrength * (0.25f + 0.75f * gust) + wave * windStrength * (0.3f + 0.4f * gust);
    bend = height * height * input.shape.w;

    perpendicular = float2(-windDirection.y, windDirection.x);
    offset.xz = (windDirection * lean + perpendicular * flutter * windStrength) * bend;
    offset.y = -dot(offset.xz, offset.xz) * 0.5f;          // keep the blade roughly its length while it leans

    worldPosition = root + local + offset * size;

    output.position = mul(float4(worldPosition, 1.0f), viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    // The root sits on the ground, so the shadow pass has the ground's shadow there. (A texture read per vertex is
    // the expensive part of this shader; per pixel it is cheap, so the pixel shader reads it.)
    output.rootPosition = mul(float4(root, 1.0f), viewMatrix);
    output.rootPosition = mul(output.rootPosition, projectionMatrix);

    output.tex = input.tex;

    // Thin two-sided blades light badly with their own normals; lean them toward "up" so grass shades like the
    // ground it stands on.
    output.normal = normalize(lerp(normalize(normal), float3(0.0f, 1.0f, 0.0f), 0.65f));

    output.lightPos = normalize(lightPosition - worldPosition);
    output.tint = input.tintRank.rgb;
    output.height = height;

    return output;
}
