#version 460

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in int id;

layout (location = 0) out vec3 v_pos;
layout (location = 1) out vec3 v_norm;
layout (location = 2) out vec3 v_light_space_pos;
layout (location = 3) out flat int v_id;

uniform mat4 u_vp;
uniform mat4 u_light_vp;

#include <res/wind.glsl>
#include <res/ls2v3.glsl>

void main() {
  vec4 final = vec4(pos, 1.) * u_vp;
  v_light_space_pos = cvt_ls2v3(vec4(pos, 1.) * u_light_vp);
  v_norm = norm;
  v_pos = final.xyz;
  gl_Position = final;
  v_id = id;
}