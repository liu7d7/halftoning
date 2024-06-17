float hash(vec3 p) {
  return fract(sin(p.x * 12.56 + p.y * 8.73 + p.z * 98.06) * 59723.71035);
}