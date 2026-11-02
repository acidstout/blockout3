// Block Out III - retro screen effects (GPL v3 or later)
// All passes draw a full screen triangle. The canvas is the finished 4:3
// picture; the final pass writes it into its place in the window.

cbuffer Post : register(b0) {
  float4 srcSize;    // source texture: width, height, 1/width, 1/height
  float4 outSize;    // final viewport: width, height (pixels)
  float4 params;     // afterglow decay per frame, scanline count, blur direction (x, y)
  uint4  flags;      // x: effect bits (see FX_*)
};

#define FX_SCANLINES 1
#define FX_VIGNETTE  2
#define FX_GLOW      4
#define FX_CURVATURE 8
#define FX_MASK      16
#define FX_EGA       32

Texture2D    tex0 : register(t0);
Texture2D    tex1 : register(t1);
SamplerState linearClamp : register(s0);
SamplerState pointClamp : register(s1);

struct VSOut {
  float4 pos : SV_Position;
  float2 uv  : TEXCOORD0;
};

VSOut VS(uint id : SV_VertexID) {
  VSOut o;
  o.uv = float2((id << 1) & 2, id & 2);
  o.pos = float4(o.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
  return o;
}

// ---- Afterglow: phosphor persistence ---------------------------------------
// tex0: new picture, tex1: previous persistence buffer
float4 PersistPS(VSOut i) : SV_Target {
  float3 now = tex0.Sample(pointClamp, i.uv).rgb;
  float3 old = tex1.Sample(pointClamp, i.uv).rgb * params.x;
  return float4(max(now, old), 1.0);
}

// ---- Glow: bright parts, quarter resolution, separable blur ----------------
float4 BrightPS(VSOut i) : SV_Target {
  // 4x4 box of the full size picture (4 bilinear taps)
  float2 d = srcSize.zw;
  float3 c = tex0.Sample(linearClamp, i.uv + d * float2(-1, -1)).rgb + tex0.Sample(linearClamp, i.uv + d * float2(1, -1)).rgb
           + tex0.Sample(linearClamp, i.uv + d * float2(-1, 1)).rgb + tex0.Sample(linearClamp, i.uv + d * float2(1, 1)).rgb;
  c *= 0.25;
  float l = dot(c, float3(0.299, 0.587, 0.114));
  return float4(c * smoothstep(0.15, 0.6, l), 1.0);
}

float4 BlurPS(VSOut i) : SV_Target {
  static const float w[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };
  float2 step = params.zw * srcSize.zw * 1.5;
  float3 c = tex0.Sample(linearClamp, i.uv).rgb * w[0];
  [unroll] for (int k = 1; k < 5; k++) {
    c += tex0.Sample(linearClamp, i.uv + step * k).rgb * w[k];
    c += tex0.Sample(linearClamp, i.uv - step * k).rgb * w[k];
  }
  return float4(c, 1.0);
}

// ---- 320x200 mode: brightest pixel of each cell, nearest up -----------------
float4 LowDownPS(VSOut i) : SV_Target {
  // srcSize: the full canvas. Each 320x200 cell keeps its brightest source
  // pixel, so thin lines stay continuous and keep their colour.
  int2 a = int2(floor(i.pos.xy) * srcSize.xy / float2(320.0, 200.0));
  int2 b = int2(floor((floor(i.pos.xy) + 1.0) * srcSize.xy / float2(320.0, 200.0)));
  float3 best = 0;
  float bestL = -1;
  for (int y = a.y; y < b.y; y++)
    for (int x = a.x; x < b.x; x++) {
      float3 c = tex0.Load(int3(x, y, 0)).rgb;
      float l = dot(c, float3(0.299, 0.587, 0.114));
      if (l > bestL) { bestL = l; best = c; }
    }
  return float4(best, 1.0);
}

float4 LowUpPS(VSOut i) : SV_Target {
  return float4(tex0.Sample(pointClamp, i.uv).rgb, 1.0);
}

// ---- Final pass ------------------------------------------------------------

static const float3 EGA[16] = {
  float3(0,0,0), float3(0,0,0.667), float3(0,0.667,0), float3(0,0.667,0.667),
  float3(0.667,0,0), float3(0.667,0,0.667), float3(0.667,0.333,0), float3(0.667,0.667,0.667),
  float3(0.333,0.333,0.333), float3(0.333,0.333,1), float3(0.333,1,0.333), float3(0.333,1,1),
  float3(1,0.333,0.333), float3(1,0.333,1), float3(1,1,0.333), float3(1,1,1) };

float3 NearestEga(float3 c) {
  float best = 1e9;
  float3 r = c;
  [unroll] for (int k = 0; k < 16; k++) {
    float3 d = (c - EGA[k]) * float3(0.30, 0.59, 0.11);
    float e = dot(d, d);
    if (e < best) { best = e; r = EGA[k]; }
  }
  return r;
}

// tex0: picture (canvas or afterglow buffer), tex1: glow
float4 FinalPS(VSOut i) : SV_Target {

  uint fx = flags.x;
  float2 uv = i.uv;
  float edge = 1.0;

  if (fx & FX_CURVATURE) {
    // Barrel distortion of a 4:3 tube, black outside, rounded corners
    float2 c = (uv - 0.5) * float2(1.0, 0.75) * 2.0;
    uv = uv + (uv - 0.5) * dot(c, c) * 0.06;
    float2 q = abs(uv - 0.5) * 2.0;
    float2 corner = max(q - (1.0 - 0.04), 0.0) / 0.04;
    edge = 1.0 - smoothstep(0.8, 1.0, length(corner));
    if (any(q > 1.0)) edge = 0.0;
  }

  float3 col = tex0.Sample(linearClamp, uv).rgb;
  if (fx & FX_EGA) col = NearestEga(saturate(col));
  if (fx & FX_GLOW) col += tex1.Sample(linearClamp, uv).rgb * 0.9;

  if (fx & FX_SCANLINES) {
    // A fixed number of lines over the picture height: same look at any resolution
    float s = 0.5 + 0.5 * cos(uv.y * params.y * 6.2831853);
    col *= lerp(0.55, 1.15, s);
  }

  if ((fx & FX_MASK) && outSize.y >= 1440.0) {
    // Aperture grille: red, green, blue stripes of one pixel
    uint p = (uint)i.pos.x % 3u;
    float3 m = p == 0 ? float3(1.0, 0.72, 0.72) : (p == 1 ? float3(0.72, 1.0, 0.72) : float3(0.72, 0.72, 1.0));
    col *= m * 1.18;
  }

  if (fx & FX_VIGNETTE) {
    float2 c = (uv - 0.5) * float2(1.0, 0.75) * 2.0;
    col *= 1.0 - 0.45 * smoothstep(0.35, 1.25, length(c));
  }

  return float4(saturate(col) * edge, 1.0);
}
