
uniform sampler2D v_texture;
uniform lowp float gray;

in vec2 v_texCoord;

const vec3 lumaF = vec3(.299, .587, .114);

out vec4 fragColor;
void main() {
  /* Sample source color */
  vec4 frag = texture(v_texture, v_texCoord);

  /* Apply gray */
  float luma = dot(frag.rgb, lumaF);
  frag.rgb = mix(frag.rgb, vec3(luma), gray);

  fragColor = frag;
}
