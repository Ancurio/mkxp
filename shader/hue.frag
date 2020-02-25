
uniform sampler2D v_texture;
uniform mediump float hueAdjust;

in vec2 v_texCoord;

out vec4 fragColor;

/* Source: gamedev.stackexchange.com/a/59808/24839 */
vec3 rgb2hsv(vec3 c) {
  const vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
  vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
  vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

  float d = q.x - min(q.w, q.y);

  /* Avoid divide-by-zero situations by adding a very tiny delta.
   * Since we always deal with underlying 8-Bit color values, this
   * should never mask a real value */
  const float eps = 1.0e-10;

  return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + eps)), d / (q.x + eps), q.x);
}

vec3 hsv2rgb(vec3 c) {
  const vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
  vec4 color = texture(v_texture, v_texCoord.xy);
  vec3 hsv = rgb2hsv(color.rgb);

  hsv.x += hueAdjust;
  color.rgb = hsv2rgb(hsv);

  fragColor = color;
}
