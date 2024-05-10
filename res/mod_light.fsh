#version 460

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec3 v_norm;
layout (location = 2) in vec3 v_light_space_pos;

layout (location = 0) out vec4 f_color;

uniform vec3 u_eye;
uniform vec3 u_dark;
uniform vec3 u_light;
uniform vec3 u_light_model;
uniform float u_shine;
uniform vec2 u_light_tex_size;
uniform float u_trans;
uniform sampler2D u_light_tex;

const vec3 light_dir = vec3(-1, 2, -1);
const float gauss_3x3[3][3] = {
{1./16., 1./8., 1./16.},
{1./8., 1./4., 1./8.},
{1./16., 1./8., 1./16.},
};
const float gauss_5x5[5][5] = {
{0.003, 0.013, 0.022, 0.013, 0.003},
{0.013, 0.059, 0.097, 0.059, 0.013},
{0.022, 0.097, 0.159, 0.097, 0.022},
{0.013, 0.059, 0.097, 0.059, 0.013},
{0.003, 0.013, 0.022, 0.013, 0.003},
};

float shadow_calc() {
  vec3 proj = v_light_space_pos;
  float shadow = 0.;
  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      float closest_depth = texture(u_light_tex, proj.xy + u_light_tex_size * vec2(float(i), float(j))).r;
      float current_depth = proj.z;
      float bias = max(0.0003 * (1.0 - dot(v_norm, light_dir)), 0.0002);
      shadow += (current_depth - bias > closest_depth ? 1.0 : 0.0) * gauss_3x3[i + 1][j + 1];
    }
  }

  return shadow;
}

vec3 light_calc(vec3 N, float transmission) {
  vec3 L = normalize(light_dir);
  vec3 V = normalize(u_eye - v_pos.xyz);
  vec3 R = reflect(-L, N);
  float lambert = max(dot(N, L), 0.0);
  float specular = pow(max(dot(R, V), 0.0), u_shine);
  float amt = clamp(u_light_model.x + (1. - shadow_calc()) * (transmission * lambert * u_light_model.y + specular * u_light_model.z), 0., 1.);
  return mix(u_dark, u_light, smoothstep(0., 1., amt));
}

void main() {
  vec3 norm = v_norm;
  float transmission = 1.;
  if (!gl_FrontFacing) {
    if (u_trans > 0.0001) {
      transmission = u_trans;
    } else {
      norm = -norm;
    }
  }

  f_color = vec4(light_calc(norm, transmission), 1.);
}