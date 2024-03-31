#version 460

layout (location = 0) in vec2 v_uv;

layout (location = 0) out vec4 f_color;

uniform sampler2D u_cmyk;
uniform float u_dots_per_line;
uniform vec2 u_scr_size;

vec2 u_inv_scr_size = 1. / u_scr_size;
float u_dot_size = u_scr_size.x / u_dots_per_line;
float u_inv_dot_size = 1. / u_dot_size;
float neg_dot_size = -u_dot_size, dot_size_x2 = 2 * u_dot_size;

// v03 v13 v23 v33
// v02 v12 v22 v32
// v01 v11 v21 v31
// v00 v10 v20 v30

vec4 colors[4][4];
vec2 poses[4][4];

// vertices in screen space, rotated!
bool does_fill_quad(vec2 self, int x, int y, float theta, int index) {
  vec2 v0 = poses[x][y], v1 = poses[x][y + 1], v2 = poses[x + 1][y + 1], v3 = poses[x + 1][y];
  vec4 c0 = colors[x][y], c1 = colors[x][y + 1], c2 = colors[x + 1][y + 1], c3 = colors[x + 1][y];
  vec2 mid = (v0 + v1 + v2 + v3) * 0.25;

  vec4 cmyk = (c0 + c1 + c2 + c3) * 0.25;

  float this_dot_rad_sq = pow(u_dot_size, 2) * cmyk[index] / 2.95;
  vec2 to_dot = mid - self;

  return dot(to_dot, to_dot) < this_dot_rad_sq;
}

vec3 subtract_for(float theta, int index, vec3 subtraction) {
  // first: rotate by negative of plane angle
  vec2 screen_space = v_uv * u_scr_size;
  vec2 screen_space_back_rotated = screen_space * mat2(cos(-theta), -sin(-theta), sin(-theta), cos(-theta));
  vec2 dot_space_back_rotated = screen_space_back_rotated * u_inv_dot_size;

  mat2 rot = mat2(cos(theta), -sin(theta), sin(theta), cos(theta));

  poses[1][1] = (floor(dot_space_back_rotated) * u_dot_size);
  poses[0][0] = vec2(poses[1][1].x + neg_dot_size, poses[1][1].y + neg_dot_size) * rot;
  poses[0][1] = vec2(poses[1][1].x + neg_dot_size, poses[1][1].y) * rot;
  poses[0][2] = vec2(poses[1][1].x + neg_dot_size, poses[1][1].y + u_dot_size) * rot;
  poses[0][3] = vec2(poses[1][1].x + neg_dot_size, poses[1][1].y + dot_size_x2) * rot;
  poses[1][0] = vec2(poses[1][1].x, poses[1][1].y + neg_dot_size) * rot;
  poses[1][2] = vec2(poses[1][1].x, poses[1][1].y + u_dot_size) * rot;
  poses[1][3] = vec2(poses[1][1].x, poses[1][1].y + dot_size_x2) * rot;
  poses[2][0] = vec2(poses[1][1].x + u_dot_size, poses[1][1].y + neg_dot_size) * rot;
  poses[2][1] = vec2(poses[1][1].x + u_dot_size, poses[1][1].y) * rot;
  poses[2][2] = vec2(poses[1][1].x + u_dot_size, poses[1][1].y + u_dot_size) * rot;
  poses[2][3] = vec2(poses[1][1].x + u_dot_size, poses[1][1].y + dot_size_x2) * rot;
  poses[3][0] = vec2(poses[1][1].x + dot_size_x2, poses[1][1].y + neg_dot_size) * rot;
  poses[3][1] = vec2(poses[1][1].x + dot_size_x2, poses[1][1].y) * rot;
  poses[3][2] = vec2(poses[1][1].x + dot_size_x2, poses[1][1].y + u_dot_size) * rot;
  poses[3][3] = vec2(poses[1][1].x + dot_size_x2, poses[1][1].y + dot_size_x2) * rot;
  poses[1][1] = poses[1][1] * rot;

  colors[0][0] = texture(u_cmyk, poses[0][0] * u_inv_scr_size);
  colors[0][1] = texture(u_cmyk, poses[0][1] * u_inv_scr_size);
  colors[0][2] = texture(u_cmyk, poses[0][2] * u_inv_scr_size);
  colors[0][3] = texture(u_cmyk, poses[0][3] * u_inv_scr_size);
  colors[1][0] = texture(u_cmyk, poses[1][0] * u_inv_scr_size);
  colors[1][1] = texture(u_cmyk, poses[1][1] * u_inv_scr_size);
  colors[1][2] = texture(u_cmyk, poses[1][2] * u_inv_scr_size);
  colors[1][3] = texture(u_cmyk, poses[1][3] * u_inv_scr_size);
  colors[2][0] = texture(u_cmyk, poses[2][0] * u_inv_scr_size);
  colors[2][1] = texture(u_cmyk, poses[2][1] * u_inv_scr_size);
  colors[2][2] = texture(u_cmyk, poses[2][2] * u_inv_scr_size);
  colors[2][3] = texture(u_cmyk, poses[2][3] * u_inv_scr_size);
  colors[3][0] = texture(u_cmyk, poses[3][0] * u_inv_scr_size);
  colors[3][1] = texture(u_cmyk, poses[3][1] * u_inv_scr_size);
  colors[3][2] = texture(u_cmyk, poses[3][2] * u_inv_scr_size);
  colors[3][3] = texture(u_cmyk, poses[3][3] * u_inv_scr_size);

  if (does_fill_quad(screen_space, 0, 0, theta, index) ||
      does_fill_quad(screen_space, 0, 1, theta, index) ||
      does_fill_quad(screen_space, 0, 2, theta, index) ||
      does_fill_quad(screen_space, 1, 0, theta, index) ||
      does_fill_quad(screen_space, 1, 1, theta, index) ||
      does_fill_quad(screen_space, 1, 2, theta, index) ||
      does_fill_quad(screen_space, 2, 0, theta, index) ||
      does_fill_quad(screen_space, 2, 1, theta, index) ||
      does_fill_quad(screen_space, 2, 2, theta, index)) {
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