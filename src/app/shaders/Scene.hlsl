// Block Out III - scene shader (GPL v3 or later)
// Reproduces the OpenGL 1.x fixed function look of BlockOut II:
// unlit primitives use the material ambient colour, lit ones the GL lighting
// equation (one local light, global ambient 1, per-vertex, infinite viewer).
// Lines are expanded to screen space quads of constant pixel width.

struct Material {
  float4 ambient;
  float4 diffuse;
  float4 specular;   // rgb, shininess
};

cbuffer Frame : register(b0) {
  row_major float4x4 proj;     // GL projection (z in -w..w)
  float4   viewport;           // width, height (pixels), line half width (pixels), unused
  float4   lightEye;           // light position in eye space
  Material mats[17];     // MAT_COUNT
};

cbuffer Object : register(b1) {
  row_major float4x4 modelView;
};

struct VSIn {
  float3 pos  : POSITION;
  float3 aux  : NORMAL;     // normal, or the other end of a line
  uint   info : TEXCOORD0;  // material | kind<<8 | end<<12 | side<<13
};

struct VSOut {
  float4 pos   : SV_Position;
  float4 color : COLOR0;
};

float4 ToD3D(float4 clip) {
  clip.z = 0.5 * (clip.z + clip.w);
  return clip;
}

VSOut VS(VSIn i) {

  VSOut o;
  Material m = mats[i.info & 0xFF];
  uint kind = (i.info >> 8) & 0xF;
  float4 eye = mul(modelView, float4(i.pos, 1.0));
  float4 clip = mul(proj, eye);
  o.color = m.ambient;

  if (kind == 1) {
    // GL lighting: emission(0) + ambient*global(1) + diffuse + specular
    float3 n = normalize(mul((float3x3)modelView, i.aux));
    float3 l = normalize(lightEye.xyz - eye.xyz);
    float nl = dot(n, l);
    float3 c = m.ambient.rgb + max(nl, 0.0) * m.diffuse.rgb;
    if (nl > 0.0 && m.specular.w > 0.0) {
      float3 h = normalize(l + float3(0.0, 0.0, 1.0));
      c += pow(max(dot(n, h), 0.0), m.specular.w) * m.specular.rgb;
    }
    o.color = float4(saturate(c), m.diffuse.a);
  }
  else if (kind == 2) {
    // Line: P0 -> P1 is the same direction for every vertex of the quad, so
    // all quads have the same (front facing) winding
    uint end = (i.info >> 12) & 1;
    float side = ((i.info >> 13) & 1) ? 1.0 : -1.0;
    float4 other = mul(proj, mul(modelView, float4(i.aux, 1.0)));
    float4 c0 = end ? other : clip;
    float4 c1 = end ? clip : other;
    float2 s0 = c0.xy / c0.w * viewport.xy;
    float2 s1 = c1.xy / c1.w * viewport.xy;
    float2 d = s1 - s0;
    float len = length(d);
    d = len > 1e-4 ? d / len : float2(1.0, 0.0);
    float2 perp = float2(-d.y, d.x);
    // square caps close the gaps where lines meet
    float2 offset = (perp * side + d * (end ? 1.0 : -1.0)) * viewport.z;
    clip.xy += offset * 2.0 / viewport.xy * clip.w;
    clip.z -= 1e-4 * clip.w;   // lines win over the faces they lie on
  }

  o.pos = ToD3D(clip);
  return o;

}

float4 PS(VSOut i) : SV_Target {
  return i.color;
}
