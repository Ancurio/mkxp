
uniform float alpha;

varying vec4 v_color;

void main()
{
	vec4 frag = vec4(0, 0, 0, 1);

	frag.rgb = mix(vec3(0, 0, 0), v_color.rgb, alpha);

	gl_FragColor = frag;
}
