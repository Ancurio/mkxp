/* Fragment shader dealing with transitions */

uniform sampler2D currentScene;
uniform sampler2D frozenScene;
uniform sampler2D transMap;
/* Normalized */
uniform float prog;
/* Vague [0, 512] normalized */
uniform float vague;

varying vec2 v_texCoord;

void main()
{
    float transV = texture2D(transMap, v_texCoord).r;
    float cTransV = clamp(transV, prog, prog+vague);
    lowp float alpha = (cTransV - prog) / vague;
    
    vec4 newFrag = texture2D(currentScene, v_texCoord);
    vec4 oldFrag = texture2D(frozenScene, v_texCoord);
    
    gl_FragColor = mix(newFrag, oldFrag, alpha);
}
