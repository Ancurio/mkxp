
uniform lowp float alpha;

varying lowp vec4 v_color;

void main()
{
	gl_FragColor = vec4(v_color.rgb * alpha, 1);
}
