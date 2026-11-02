// Block Out III - 2D overlay (GPL v3 or later)
// Vertices in canvas pixels; the font atlas holds coverage in its red channel.

cbuffer Ui : register(b0) {
  float4 canvas;   // width, height
};

Texture2D    font : register(t0);
SamplerState pointClamp : register(s0);

struct VSIn {
  float2 pos   : POSITION;
  float2 uv    : TEXCOORD0;
  float4 color : COLOR0;
};

struct VSOut {
  float4 pos   : SV_Position;
  float2 uv    : TEXCOORD0;
  float4 color : COLOR0;
};

VSOut VS(VSIn i) {
  VSOut o;
  o.pos = float4(i.pos.x / canvas.x * 2.0 - 1.0, 1.0 - i.pos.y / canvas.y * 2.0, 0.0, 1.0);
  o.uv = i.uv;
  o.color = i.color;
  return o;
}

float4 PS(VSOut i) : SV_Target {
  return float4(i.color.rgb, i.color.a * font.Sample(pointClamp, i.uv).r);
}
