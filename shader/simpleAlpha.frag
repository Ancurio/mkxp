
uniform sampler2D v_texture;

in vec2 v_texCoord;
in lowp vec4 v_color;

out vec4 fragColor;
void main() {
  fragColor = texture(v_texture, v_texCoord);
  fragColor.a *= v_color.a;
}
