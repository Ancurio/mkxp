
uniform sampler2D texture;

uniform vec4 tone;

uniform float opacity;
uniform vec4 color;

uniform float bushDepth;
uniform float bushOpacity;

varying vec2 v_texCoord;

const vec3 lumaF = vec3(.299, .587, .114);

void main()
{
	/* Sample source color */
	vec4 frag = texture2D(texture, v_texCoord);
	
	/* Apply gray */
	float luma = dot(frag.rgb, lumaF);
	frag.rgb = mix(frag.rgb, vec3(luma), tone.w);
	
	/* Apply tone */
	frag.rgb += tone.rgb;

	/* Apply opacity */
	frag.a *= opacity;
	
	/* Apply color */
	frag.rgb = mix(frag.rgb, color.rgb, color.a);

	/* Apply bush alpha by mathematical if */
	float underBush = float(v_texCoord.y < bushDepth);
	frag.a *= clamp(bushOpacity + underBush, 0, 1);
	
	gl_FragColor = frag;
}
