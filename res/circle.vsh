#version 460

layout (location = 0) in vec2 pos;

layout (location = 0) out vec2 v_pos;

uniform mat4 u_proj;

void main() {
  v_pos = pos;
  gl_Position = vec4(v_pos, 0, 1) * u_proj;
}