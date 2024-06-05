#version 460

layout (location = 0) in vec2 v_uv;
layout (location = 1) in vec4 v_col;

layout (location = 0) out vec4 f_color;

uniform sampler2D u_tex;

void main() {
  vec4 tex_color = texture(u_tex, v_uv);
  if (tex_color.r < 0.0001) discard;

  f_color = vec4(vec3(1.), tex_color.r) * v_col;
}