
uniform sampler2D source;
uniform sampler2D destination;

uniform vec4 subRect;

uniform float opacity;

void main()
{
	vec2 coor = gl_TexCoord[0].xy;
	vec2 dstCoor = (coor - subRect.xy) * subRect.zw;

	vec4 srcFrag = texture2D(source, coor);
	vec4 dstFrag = texture2D(destination, dstCoor);

	vec4 resFrag;

	float ab = opacity;
	const float as = srcFrag.a;
	const float ad = dstFrag.a;

	const float at = ab*as;
	resFrag.a = at + ad - ad*at;

	resFrag.rgb = mix(dstFrag.rgb, srcFrag.rgb, ab*as);
//	resFrag.rgb /= (resFrag.a);
	resFrag.rgb = mix(srcFrag.rgb, resFrag.rgb, ad*resFrag.a);

	gl_FragColor = resFrag;
}
