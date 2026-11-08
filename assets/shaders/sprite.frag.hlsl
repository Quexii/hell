[[vk::binding(0, 2)]]
Sampler2D texture0 : register(t0, space2) : register(s0, space2);

struct InstanceOut {
    uint tint;
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
    float4 tint = UnpackRGBA8(input.instance.tint);

    return mask * tint;
}

