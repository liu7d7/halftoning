#version 460
in vec2 v_uv;

out vec4 f_color;

uniform sampler2D u_tex0;
uniform vec3 u_pal[256];
uniform int u_pal_size;

const int idx_mat4x4[16] = int[](
  0,  8,  2,  10,
  12, 4,  14, 6,
  3,  11, 1,  9,
  15, 7,  13, 5
);

float lightness_step(float l) {
  const float lightness_steps = 4.0;
  return floor((0.5 + l * lightness_steps)) / lightness_steps;
}

float idx_val() {
  int x = int(mod(gl_FragCoord.x, 4));
  int y = int(mod(gl_FragCoord.y, 4));
  return idx_mat4x4[x + y * 4] / 16.;
}

float hue_dist(float h1, float h2) {
  float diff = abs(h1 - h2);
  return min(abs(1. - diff), diff);
}

vec3 to_hsl(vec3 c) {
  vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
  vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
  vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

  float d = q.x - min(q.w, q.y);
  float e = 1.0e-10;
  return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 to_rgb(vec3 c) {
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3[2] closest_cols(float hue) {
  vec3 ret[2];
  vec3 closest = vec3(-2., 0., 0.);
  vec3 snd_closest = vec3(-2., 0., 0.);
  vec3 temp;
  for (int i = 0; i < u_pal_size; i++) {
    temp = to_hsl(u_pal[i]);
    float tmp_dist = hue_dist(hue, temp.r);
    if (tmp_dist < hue_dist(closest.x, hue)) {
      snd_closest = closest;
      closest = temp;
    } else if (tmp_dist < hue_dist(snd_closest.x, hue)) {
      snd_closest = temp;
    }
  }
  ret[0] = closest;
  ret[1] = snd_closest;
  return ret;
}

vec3 dither(vec3 col) {
  vec3 hsl = to_hsl(col);
  vec3 colors[2] = closest_cols(hsl.x);
  float hueDiff = hue_dist(hsl.x, colors[0].x) / hue_dist(colors[1].x, colors[0].x);

  float l1 = lightness_step(max((hsl.z - 0.125), 0.0));
  float l2 = lightness_step(min((hsl.z + 0.124), 1.0));
  float lightnessDiff = (hsl.z - l1) / (l2 - l1);

  vec3 res = hueDiff < idx_val() ? colors[0] : colors[1];
  res.z = (lightnessDiff < idx_val()) ? l1 : l2;
  return to_rgb(res);
}

void main() {
  f_color = vec4(dither(texture(u_tex0, v_uv).rgb), 1.);
}