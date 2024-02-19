#version 460

layout (location = 0) in vec2 v_uv;

layout (location = 0) out vec4 f_color;

uniform sampler2D u_tex;

void main() {
  vec3 color = texture(u_tex, v_uv).rgb;
  float r = color.r, g = color.g, b = color.b;
  float k = 1 - max(r, max(g, b));
  float light = 1 - k;
  float c = (light - r) / light;
  float m = (light - g) / light;
  float y = (light - b) / light;

  f_color = vec4(c, m, y, k);
}