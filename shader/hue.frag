
uniform sampler2D texture;
uniform lowp mat3 rotationMat;

varying vec2 v_texCoord;

void main ()
{
	vec4 color = texture2D (texture, v_texCoord.xy);

	gl_FragColor = vec4(rotationMat * color.rgb, color.a);
}
