#version 460

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec3 v_norm;
//layout(location = 2) in vec4 v_light_space_pos;

layout (location = 0) out vec4 f_color;

uniform vec3 u_eye;
uniform vec3 u_dark;
uniform vec3 u_light;
uniform vec3 u_light_model;
uniform float u_shine;
//uniform bool u_has_norm_tex;
//uniform sampler2D u_norm_tex;
//uniform bool u_has_alpha_tex;
//uniform sampler2D u_alpha_tex;
//uniform highp uint u_id;
//uniform sampler2D u_light_tex;

const vec3 light_dir = vec3(-1, 2, -1);

//float shadow_calc() {
//  vec3 proj = (v_light_space_pos.xyz / v_light_space_pos.w) * 0.5 + 0.5;
//  float closest_depth = texture(u_light_tex, proj.xy).r;
//  float current_depth = proj.z;
//  float bias = max(0.0003 * (1.0 - dot(v_norm, light_dir)), 0.0002);
//  float shadow = current_depth - bias > closest_depth ? 1.0 : 0.0;
//  return shadow;
//}

vec3 light_calc(vec3 N) {
  vec3 L = normalize(light_dir);
  vec3 V = normalize(u_eye - v_pos.xyz);
  vec3 R = reflect(-L, N);
  float lambert = max(dot(N, L), 0.0);
  float specular = pow(max(dot(R, V), 0.0), u_shine);
  float amt = clamp(u_light_model.x + /* (1. - shadow_calc()) * */ (lambert * u_light_model.y + specular * u_light_model.z), 0., 1.);
  return mix(u_dark, u_light, smoothstep(0., 1., amt));
}

void main() {
  vec3 norm = v_norm;
  if (!gl_FrontFacing) {
    norm = -norm;
  }

  f_color = vec4(light_calc(norm), 1.);
}