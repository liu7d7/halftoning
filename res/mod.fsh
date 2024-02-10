#version 460

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec3 v_norm;
layout (location = 2) in vec2 v_tex;

layout (location = 0) out vec4 f_color;

uniform vec3 u_eye;

void main() {
//  f_color = vec4(v_norm, 1.);
  f_color = vec4(v_norm * 0.5 + 0.5, 1.);
//  f_color = vec4(vec3(0), 1.);
}