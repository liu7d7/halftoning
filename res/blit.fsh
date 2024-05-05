#version 460

layout (location = 0) in vec2 v_uv;

layout (location = 0) out vec4 f_color;

uniform sampler2D u_tex;

void main() {
  f_color = vec4(vec3(texture(u_tex, v_uv).r), 1.);
}