#version 460

layout (location = 0) in vec3 pos;
layout (location = 2) in mat4 model;

uniform mat4 u_vp;

#include <res/wind.glsl>

void main() {
  gl_Position = vec4(do_wind(pos), 1.) * model * u_vp;
}