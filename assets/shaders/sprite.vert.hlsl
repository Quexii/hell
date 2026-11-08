[[vk::binding(0, 1)]]
cbuffer Matrices : register(b0, space1) {
    float4x4 mvp;
};

struct SpriteRegion {
    float4 uvRect;
};

[[vk::binding(0, 0)]]
StructuredBuffer<SpriteRegion> spriteRegions : register(t0, space0);

struct Instance {
    float2 position, scale;
    float rotation;
    uint spriteID, tint;
};

struct InstanceOut {
    uint tint;
};

struct VSInput {
    float3 position : TEXCOORD0;
    float2 uv : TEXCOORD1;
    Instance instance;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    InstanceOut instance;
};

VSOutput main(VSInput input, uint instanceID : SV_VulkanInstanceID) {
    SpriteRegion region = spriteRegions[input.instance.spriteID];

    float2 p = input.position.xy * input.instance.scale;

    float c = cos(input.instance.rotation);
    float s = sin(input.instance.rotation);

    p = float2(
        p.x * c - p.y * s,
        p.x * s + p.y * c);

    p += input.instance.position;

    VSOutput output;
    output.position = mul(mvp, float4(p, input.position.z, 1.0));
    output.uv = lerp(
        region.uvRect.xy,
        region.uvRect.zw,
        input.uv);
    output.instance.tint = input.instance.tint;
    return output;
}

