#version 460

layout (location = 0) in vec3 pos;
layout (location = 2) in ivec4 boneIds;
layout (location = 3) in vec4 weights;

uniform mat4 u_vp;
uniform mat4 u_light_vp;
uniform mat4 u_model;
uniform int u_id;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 u_final_mats[MAX_BONES];

void main()
{
  mat4 bone_transform = u_final_mats[boneIds[0]] * weights[0];
  bone_transform += u_final_mats[boneIds[1]] * weights[1];
  bone_transform += u_final_mats[boneIds[2]] * weights[2];
  bone_transform += u_final_mats[boneIds[3]] * weights[3];

  vec4 pos_l = vec4(pos, 1.) * bone_transform * u_model;

  vec4 final = pos_l * u_vp;
  gl_Position = final;
}