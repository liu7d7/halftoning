uniform float u_wind;
uniform float u_time;

const float pi = 3.1415926;

float wind_fn(float off) {
  return sin(u_time * 1.5 + off) + 0.75 * sin(u_time * 2 * 1.5 + pi + off) + 0.5 * sin(u_time * 3 * 1.5 + pi / 2.f + off);
}

vec3 do_wind(vec3 pos) {
  const vec3 wind_dir = normalize(vec3(1., 0.3, 1.));
  float base = sin(pos.x + pos.z);
  return pos + wind_dir * u_wind * vec3(wind_fn(base), wind_fn(pi * 2.3f + base), wind_fn(pi + base)) * sin(pos.y / 2.);
}
