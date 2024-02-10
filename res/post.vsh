#version 460

layout (location = 0) in vec2 uv;

layout (location = 0) out vec2 v_uv;

void main() {
  gl_Position = vec4(uv * 2 - 1, 0, 1);
  v_uv = uv;
}