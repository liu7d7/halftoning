#version 460

layout (location = 0) in vec2 v_uv;

layout (location = 0) out vec4 f_color;

uniform isampler2D u_tex;
uniform int u_id;

void main() {
  vec2 one_texel = 1. / vec2(textureSize(u_tex, 0));
  int mid = texture(u_tex, v_uv).r;
  if (mid == u_id) {
    f_color = vec4(0.);
    return;
  }

  for (int i = -3; i <= 3; i++) {
    for (int j = -3; j <= 3; j++) {
      if (i == 0 && j == 0) continue;
      vec2 uv = v_uv + one_texel * vec2(float(i), float(j));
      int res = texture(u_tex, uv).r;
      if (res == u_id) {
        f_color = vec4(vec3(sqrt(i * i + j * j) / 3.), 1.);
        return;
      }
    }
  }

  f_color = vec4(0.);
}