[[vk::binding(0, 2)]]
Sampler2D texture0 : register(t0, space2) : register(s0, space2);

enum {
  UI_DRAW_SOLID,
  UI_DRAW_TEXTURE,
  UI_DRAW_TEXT
} UIDrawMode;

struct VSOutput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : TEXCOORD1;
    uint mode : TEXCOORD2;
};

float4 main(VSOutput input) : SV_Target0 {
    float4 color;

    if (input.mode == UI_DRAW_SOLID)
      color = input.color;
    else if (input.mode == UI_DRAW_TEXTURE)
      color = input.color * texture0.Sample(input.uv);
    else
      color = float4(input.color.rgb, texture0.Sample(input.uv).r * input.color.a);

    return color;
}

