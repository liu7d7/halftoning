#version 460

uniform sampler2D u_tex;
uniform sampler2D u_depth;
uniform int u_size;
uniform float u_separation;
uniform vec2 u_tex_size;
uniform float u_min_depth;
uniform float u_max_depth;
uniform float u_near;
uniform float u_far;

layout (location = 0) in vec2 v_uv;

layout (location = 0) out vec4 f_color;

float linearize_depth(float d) {
  return u_near * u_far / (u_far + d * (u_near - u_far));
}

void main() {
  float minThreshold = 0.2;
  float maxThreshold = 0.5;

  f_color = texture(u_tex, v_uv);

  float d = linearize_depth(texture(u_depth, v_uv).r) / u_far;
  float rat = smoothstep(0., 1., (d - u_min_depth) / (u_max_depth - u_min_depth));

  int size = int(ceil(rat * float(u_size)));

  float mx = 0.0;
  vec4 cmx = f_color;

  for (int i = -size; i <= size; ++i) {
    for (int j = -size; j <= size; ++j) {
      if (!(distance(vec2(i, j), vec2(0, 0)) <= size)) { continue; }

      vec4 c = texture(u_tex, v_uv + (vec2(i, j) * u_separation * u_tex_size));

      float mxt = dot(c.rgb, vec3(0.3, 0.59, 0.11));

      if (mxt > mx) {
        mx = mxt;
        cmx = c;
      }
    }
  }

  f_color.rgb = mix(f_color.rgb , cmx.rgb , smoothstep(minThreshold, maxThreshold, mx));
}