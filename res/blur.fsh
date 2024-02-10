#version 460

layout (location = 0) in vec2 v_uv;

layout (location = 0) out vec4 f_color;

uniform vec2 u_scr_size;
uniform sampler2D u_tex;

void main() {
  float tau = 6.28318530718;

  const float directions = 16.0;
  const float quality = 3.0;
  const float size = 4.0;

  vec2 rad = size / u_scr_size;

  vec4 color = texture(u_tex, v_uv);

  for (float d = 0.0; d < tau; d += tau / directions) {
    for (float i = 1.0 / quality; i <= 1.0; i += 1.0 / quality) {
      color += texture(u_tex, v_uv + vec2(cos(d), sin(d)) * rad * i);
    }
  }

  color /= quality * directions - 15.0;
  f_color = color;
}