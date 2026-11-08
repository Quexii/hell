[[vk::binding(0, 1)]]
cbuffer Matrices : register(b0, space1) {
    float4x4 model, view, projection;
};

struct VSInput {
    float2 position : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float4 color : TEXCOORD2;
    uint mode : TEXCOORD3;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : TEXCOORD1;
    uint mode : TEXCOORD2;
};

VSOutput main(VSInput input) {
    VSOutput output;
    output.position = mul(
        projection,
        mul(view, mul(model, float4(input.position, 0.0, 1.0))));
    output.uv = input.uv;
    output.color = input.color;
    output.mode = input.mode;
    return output;
}

