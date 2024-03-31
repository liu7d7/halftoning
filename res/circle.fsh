#version 460

layout (location = 0) in vec2 v_pos;

layout (location = 0) out vec4 f_color;

uniform vec2 u_center;
uniform float u_rad;
uniform vec4 u_color;

void main() {
  float aa = min(u_rad / 10.f, 3), d = distance(v_pos, u_center);

  if (d > u_rad + aa) {
    discard;
  }

  if (d > u_rad && d <= u_rad + aa) {
    f_color = vec4(u_color.rgb, smoothstep(1, 0, (d - u_rad) / aa) * u_color.a);
    return;
  }

  f_color = u_color;
}