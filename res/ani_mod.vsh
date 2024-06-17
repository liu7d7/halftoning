#version 460

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in ivec4 boneIds;
layout (location = 3) in vec4 weights;

uniform mat4 u_vp;
uniform mat4 u_light_vp;
uniform mat4 u_model;
uniform int u_id;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 u_final_mats[MAX_BONES];

layout (location = 0) out vec3 v_pos;
layout (location = 1) out vec3 v_norm;
layout (location = 2) out vec3 v_light_space_pos;
layout (location = 3) out flat int v_id;

void main()
{
  mat4 bone_transform = u_final_mats[boneIds[0]] * weights[0];
  if (boneIds[1] != -1) bone_transform += u_final_mats[boneIds[1]] * weights[1];
  if (boneIds[2] != -1) bone_transform += u_final_mats[boneIds[2]] * weights[2];
  if (boneIds[3] != -1) bone_transform += u_final_mats[boneIds[3]] * weights[3];

  vec4 pos_l = vec4(pos, 1.) * bone_transform * u_model;

  vec4 final = pos_l * u_vp;
  gl_Position = final;
  v_pos = final.xyz;
  vec4 light = pos_l * u_light_vp;

  v_norm = normalize(norm * mat3(transpose(inverse(bone_transform))) * mat3(transpose(inverse(u_model))));

  v_light_space_pos = light.xyz;
  v_id = u_id;
}