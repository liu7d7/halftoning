#version 460

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec2 tex;
layout (location = 3) in mat4 model;

layout (location = 0) out vec3 v_pos;
layout (location = 1) out vec3 v_norm;
layout (location = 2) out vec2 v_tex;

uniform mat4 u_proj;
uniform mat4 u_look;
uniform mat4 u_model_no_scale;

void main() {
  vec4 final = vec4(pos, 1.) * model * u_look * u_proj;
  v_norm = normalize(norm * mat3(transpose(inverse(model))));
  v_pos = final.xyz;
  v_tex = tex;
  gl_Position = final;
}