#version 460

layout (location = 0) in vec2 pos;

uniform mat4 u_proj;

void main() {
  gl_Position = vec4(pos, 0, 1) * u_proj;
}