#version 460

layout (location = 0) in vec2 v_uv;

layout (location = 0) out vec4 f_color;

#define CURVATURE 4.2

#define BLUR .021

#define CA_AMT 1.024

uniform float u_aspect;
uniform float u_lores;
uniform sampler2D u_tex0;

void main() {
  vec2 uv = v_uv;

  //curving
  vec2 crtUV = uv * 2. - 1.;
  vec2 offset = crtUV.yx / CURVATURE;
  crtUV += crtUV * offset * offset;
  crtUV = crtUV * .5 + .5;

  vec2 uv525 = crtUV * u_aspect * u_lores;

  vec2 edge = smoothstep(0., BLUR, crtUV) * (1. - smoothstep(1. - BLUR, 1., crtUV));

  //chromatic abberation
  f_color.rgb = vec3(
    texture(u_tex0, (crtUV - .5) * CA_AMT + .5).r,
    texture(u_tex0, crtUV).g,
    texture(u_tex0, (crtUV - .5) / CA_AMT + .5).b
  ) * edge.x * edge.y;

  f_color.rgb *= (sin(uv525.y * 3.1415926 / 2.f) * 0.3 + 1.);

  f_color.a = 1;

//  f_color = texture(u_tex0, v_uv);
}