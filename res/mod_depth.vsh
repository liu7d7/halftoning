#version 460

layout (location = 0) in vec3 pos;

uniform mat4 u_vp;
uniform mat4 u_model;

void main() {
  gl_Position = vec4(pos, 1.) * u_model * u_vp;
}