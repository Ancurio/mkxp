
uniform sampler2D texture;

varying vec2 v_texCoord;
varying lowp vec4 v_color;

void main()
{
	gl_FragColor = texture2D(texture, v_texCoord);
	gl_FragColor.a *= v_color.a;
}
