#version 460

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;

layout (location = 0) out vec3 v_pos;
layout (location = 1) out vec3 v_norm;
layout (location = 2) out vec4 v_light_space_pos;

uniform mat4 u_vp;
uniform mat4 u_light_vp;
uniform mat4 u_model;
uniform mat4 u_model_no_scale;

void main() {
  vec4 final = vec4(pos, 1.) * u_model * u_vp;
  v_light_space_pos = vec4(pos, 1.) * u_model * u_light_vp;
  v_norm = norm * mat3(transpose(inverse(u_model_no_scale)));
  v_pos = final.xyz;
  gl_Position = final;
}