#version 460

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec4 col;

layout (location = 0) out vec2 v_uv;
layout (location = 1) out vec4 v_col;

uniform mat4 u_proj;

void main() {
  v_uv = uv;
  v_col = col;
  gl_Position = vec4(pos, 1.) * u_proj;
}