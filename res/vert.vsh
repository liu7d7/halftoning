#version 460

layout (location = 0) in vec3 pos;

layout (location = 0) out vec3 v_pos;

uniform mat4 u_proj;
uniform mat4 u_look;

void main() {
  vec4 final = vec4(pos, 1.) * u_look * u_proj;
  v_pos = final.xyz;
  gl_Position = final;
}