
uniform sampler2D texture;

varying vec2 v_texCoord;
varying vec2 v_blurCoord[2];

void main()
{
	lowp vec4 frag = vec4(0, 0, 0, 0);

	frag += texture2D(texture, v_texCoord);
	frag += texture2D(texture, v_blurCoord[0]);
	frag += texture2D(texture, v_blurCoord[1]);

	gl_FragColor = frag / 3.0;
}
