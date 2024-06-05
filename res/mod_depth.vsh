#version 460

layout (location = 0) in vec3 pos;

uniform mat4 u_vp;
uniform mat4 u_model;

#include <res/wind.glsl>

void main() {
  gl_Position = vec4(do_wind(pos), 1.) * u_model * u_vp;
}