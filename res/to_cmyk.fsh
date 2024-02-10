#version 460

layout (location = 0) in vec2 v_uv;

layout (location = 0) out vec4 f_color;

uniform sampler2D u_tex;

void main() {
  vec3 color = texture(u_tex, v_uv).rgb;
  float r = color.r, g = color.g, b = color.b;
  float k = 1 - max(r, max(g, b));
  float c = (1 - r - k) / (1 - k);
  float m = (1 - g - k) / (1 - k);
  float y = (1 - b - k) / (1 - k);

  f_color = vec4(c, m, y, k);
}