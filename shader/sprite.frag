
uniform sampler2D texture;

uniform lowp vec4 tone;

uniform lowp float opacity;
uniform lowp vec4 color;

uniform float bushDepth;
uniform lowp float bushOpacity;

uniform sampler2D pattern;
uniform lowp float patternOpacity;
uniform bool renderPattern;

uniform bool invert;

varying vec2 v_texCoord;
varying vec2 v_patCoord;

const vec3 lumaF = vec3(.299, .587, .114);
const vec2 repeat = vec2(1, 1);

void main()
{
	/* Sample source color */
	vec4 frag = texture2D(texture, v_texCoord);
    
    /* Apply pattern */
    if (renderPattern) {
        vec4 pattfrag = texture2D(pattern, mod(v_patCoord, repeat));
        frag.rgb = mix(frag.rgb, pattfrag.rgb, pattfrag.a * patternOpacity);
    }
    
    /* Apply color inversion */
    if (invert) {
        frag.rgb = vec3(1.0 - frag.r, 1.0 - frag.g, 1.0 - frag.b);
    }
	
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
	lowp float underBush = float(v_texCoord.y < bushDepth);
	frag.a *= clamp(bushOpacity + underBush, 0.0, 1.0);
	
	gl_FragColor = frag;
}
