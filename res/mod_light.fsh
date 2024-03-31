#version 450
in vec3 v_pos;
in vec3 v_norm;

out vec4 color;

uniform vec3 u_eye;

const vec3 light_dir = normalize(vec3(1., 2.5, 1.));

vec3 light_calc(vec3 color) {
  vec3 N = v_norm;
  vec3 L = light_dir;
  vec3 V = normalize(u_eye - v_pos.xyz);
  vec3 R = reflect(-L, N);
  float lambert = max(dot(N, L), 0.0);
  float ambient = 0.;
  float amt = clamp(ambient + lambert, 0., 1.);
  return mix(color * 0.3, color, smoothstep(0., 1., amt));
}

void main() {
  vec3 col = light_calc(vec3(1.));
  color = vec4(col, 1.);
}