#version 460

layout (location = 0) in vec2 v_uv;

layout (location = 0) out vec4 f_color;

uniform sampler2D u_cmyk;
uniform float u_dots_per_line;
uniform vec2 u_scr_size;

vec2 u_inv_scr_size = 1. / u_scr_size;
float u_dot_size = u_scr_size.x / u_dots_per_line;
float u_inv_dot_size = 1. / u_dot_size;

// v03 v13 v23 v33
// v02 v12 v22 v32
// v01 v11 v21 v31
// v00 v10 v20 v30

// 0 1 2 3
// 3 4 5 6
// 6 7 8 9
// 10 11 12 13

// vertices in screen space, rotated!
bool does_fill_quad(vec2 self, vec2 v0, vec2 v1, vec2 v2, vec2 v3, float theta, int index) {
  vec2 mid = (v0 + v1 + v2 + v3) * 0.25;

  vec4 cmyk0 = texture(u_cmyk, v0 * u_inv_scr_size);
  vec4 cmyk1 = texture(u_cmyk, v1 * u_inv_scr_size);
  vec4 cmyk2 = texture(u_cmyk, v2 * u_inv_scr_size);
  vec4 cmyk3 = texture(u_cmyk, v3 * u_inv_scr_size);

  vec4 cmyk = (cmyk0 + cmyk1 + cmyk2 + cmyk3) * 0.25;

  float this_dot_rad = sqrt(pow(u_dot_size, 2) * cmyk[index] / 2.95);

  return distance(mid, self) < this_dot_rad;
}

vec3 subtract_for(float theta, int index, vec3 subtraction) {
  // first: rotate by negative of plane angle
  vec2 screen_space = v_uv * u_scr_size;
  vec2 screen_space_back_rotated = screen_space * mat2(cos(-theta), -sin(-theta), sin(-theta), cos(-theta));
  vec2 dot_space_back_rotated = screen_space_back_rotated * u_inv_dot_size;

  mat2 rot = mat2(cos(theta), -sin(theta), sin(theta), cos(theta));

  vec2 v11 = (floor(dot_space_back_rotated) * u_dot_size);
  vec2 v00 = vec2(v11.x - u_dot_size, v11.y - u_dot_size) * rot;
  vec2 v01 = vec2(v11.x - u_dot_size, v11.y) * rot;
  vec2 v02 = vec2(v11.x - u_dot_size, v11.y + u_dot_size) * rot;
  vec2 v03 = vec2(v11.x - u_dot_size, v11.y + 2 * u_dot_size) * rot;
  vec2 v10 = vec2(v11.x, v11.y - u_dot_size) * rot;
  vec2 v12 = vec2(v11.x, v11.y + u_dot_size) * rot;
  vec2 v13 = vec2(v11.x, v11.y + 2 * u_dot_size) * rot;
  vec2 v20 = vec2(v11.x + u_dot_size, v11.y - u_dot_size) * rot;
  vec2 v21 = vec2(v11.x + u_dot_size, v11.y) * rot;
  vec2 v22 = vec2(v11.x + u_dot_size, v11.y + u_dot_size) * rot;
  vec2 v23 = vec2(v11.x + u_dot_size, v11.y + 2 * u_dot_size) * rot;
  vec2 v30 = vec2(v11.x + 2 * u_dot_size, v11.y - u_dot_size) * rot;
  vec2 v31 = vec2(v11.x + 2 * u_dot_size, v11.y) * rot;
  vec2 v32 = vec2(v11.x + 2 * u_dot_size, v11.y + u_dot_size) * rot;
  vec2 v33 = vec2(v11.x + 2 * u_dot_size, v11.y + 2 * u_dot_size) * rot;
  v11 = v11 * rot;

  if (does_fill_quad(screen_space, v00, v10, v11, v01, theta, index) ||
      does_fill_quad(screen_space, v10, v20, v21, v11, theta, index) ||
      does_fill_quad(screen_space, v20, v30, v31, v21, theta, index) ||
      does_fill_quad(screen_space, v01, v11, v12, v02, theta, index) ||
      does_fill_quad(screen_space, v11, v21, v22, v12, theta, index) ||
      does_fill_quad(screen_space, v21, v31, v32, v22, theta, index) ||
      does_fill_quad(screen_space, v02, v12, v13, v03, theta, index) ||
      does_fill_quad(screen_space, v12, v22, v23, v13, theta, index) ||
      does_fill_quad(screen_space, v22, v32, v33, v23, theta, index)) {
    return subtraction;
  }

  return vec3(0.);
}

void main() {
  vec3 final_color = vec3(1.);
  final_color -= subtract_for(radians(75), 3, vec3(1.)); // k
  final_color -= subtract_for(radians(0), 2, vec3(0., 0., 1.)); // y
  final_color -= subtract_for(radians(45), 1, vec3(0., 1., 0.)); // m
  final_color -= subtract_for(radians(15), 0, vec3(1., 0., 0.)); // c
  final_color = clamp(final_color, 0, 1);

  f_color = vec4(final_color, 1.);
}