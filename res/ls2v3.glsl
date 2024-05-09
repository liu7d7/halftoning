vec3 cvt_ls2v3(vec4 ls) {
  return (ls.xyz / ls.w) * 0.5 + 0.5;
}