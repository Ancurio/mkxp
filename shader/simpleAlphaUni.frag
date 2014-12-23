
uniform sampler2D texture;
uniform lowp float alpha;

varying vec2 v_texCoord;

void main()
{
	gl_FragColor = texture2D(texture, v_texCoord);
	gl_FragColor.a *= alpha;
}
