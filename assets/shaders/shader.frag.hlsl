[[vk::binding(0, 2)]]
Sampler2D texture0 : register(t0, space2) : register(s0, space2);

struct InstanceOut {
    uint color0, color1, color2;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    InstanceOut instance;
};

float4 UnpackRGBA8(uint packed) {
    return float4(
        (packed       & 255u),
        (packed >> 8  & 255u),
        (packed >> 16 & 255u),
        (packed >> 24 & 255u)) / 255.0;
}

float4 main(VSOutput input) : SV_Target0 {
    float4 mask = texture0.Sample(input.uv);

    float3 c0 = UnpackRGBA8(input.instance.color0).rgb;
    float3 c1 = UnpackRGBA8(input.instance.color1).rgb;
    float3 c2 = UnpackRGBA8(input.instance.color2).rgb;

    float3 finalColor =
    mask.r * c0 +
    mask.g * c1 +
    mask.b * c2;

    return float4(finalColor, mask.a);
}

