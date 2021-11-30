
uniform sampler2D texture;

uniform lowp vec4 tone;

uniform lowp float opacity;
uniform lowp vec4 color;

uniform float bushDepth;
uniform lowp float bushOpacity;

uniform sampler2D pattern;
uniform int patternBlendType;
uniform lowp float patternOpacity;
uniform bool renderPattern;

uniform bool invert;

varying vec2 v_texCoord;
varying vec2 v_patCoord;

const vec3 lumaF = vec3(.299, .587, .114);
const vec2 repeat = vec2(1, 1);


// = = = = = = = = = = =
// mixing functions, from https://github.com/jamieowen/glsl-blend
// = = = = = = = = = = =

// Normal
vec3 blendNormal(vec3 base, vec3 blend) {
    return blend;
}

vec3 blendNormal(vec3 base, vec3 blend, float opacity) {
    return (blendNormal(base, blend) * opacity + base * (1.0 - opacity));
}

// Add
float blendAdd(float base, float blend) {
    return min(base+blend,1.0);
}

vec3 blendAdd(vec3 base, vec3 blend) {
    return min(base+blend,vec3(1.0));
}

vec3 blendAdd(vec3 base, vec3 blend, float opacity) {
    return (blendAdd(base, blend) * opacity + base * (1.0 - opacity));
}

// Substract
float blendSubstract(float base, float blend) {
    return max(base+blend-1.0,0.0);
}

vec3 blendSubstract(vec3 base, vec3 blend) {
    return max(base+blend-vec3(1.0),vec3(0.0));
}

vec3 blendSubstract(vec3 base, vec3 blend, float opacity) {
    return (blendSubstract(base, blend) * opacity + blend * (1.0 - opacity));
}
// = = = = = = = = = = =

void main()
{
	/* Sample source color */
	vec4 frag = texture2D(texture, v_texCoord);
    
    /* Apply pattern */
    if (renderPattern) {
        vec4 pattfrag = texture2D(pattern, mod(v_patCoord, repeat));
        if (patternBlendType == 1) {
            frag.rgb = frag.rgb = blendAdd(frag.rgb, pattfrag.rgb, pattfrag.a * patternOpacity);
        }
        else if (patternBlendType == 2) {
            frag.rgb = blendSubstract(frag.rgb, pattfrag.rgb, pattfrag.a * patternOpacity);
        }
        else {
            frag.rgb = blendNormal(frag.rgb, pattfrag.rgb, pattfrag.a * patternOpacity);
        }
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
    
    /* Apply color inversion */
    if (invert) {
        frag.rgb = vec3(1.0 - frag.r, 1.0 - frag.g, 1.0 - frag.b);
    }

	/* Apply bush alpha by mathematical if */
	lowp float underBush = float(v_texCoord.y < bushDepth);
	frag.a *= clamp(bushOpacity + underBush, 0.0, 1.0);
	
	gl_FragColor = frag;
}
