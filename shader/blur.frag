
uniform sampler2D v_texture;

in vec2 v_texCoord;
in vec2 v_blurCoord[2];

out vec4 fragColor;

void main() {
  lowp vec4 frag = vec4(0, 0, 0, 0);

  frag += texture(v_texture, v_texCoord);
  frag += texture(v_texture, v_blurCoord[0]);
  frag += texture(v_texture, v_blurCoord[1]);

  fragColor = frag / 3.0;
}
