
/* Fragment shader dealing with transitions */

uniform sampler2D currentScene;
uniform sampler2D frozenScene;
uniform sampler2D transMap;
/* Normalized */
uniform float prog;
/* Vague [0, 512] normalized */
uniform float vague;

void main()
{
    vec2 texCoor = gl_TexCoord[0].st;
    float transV = texture2D(transMap, texCoor).r;
    float cTransV = clamp(transV, prog, prog+vague);
    float alpha = (cTransV - prog) / vague;
    
    vec4 newFrag = texture2D(currentScene, texCoor);
    vec4 oldFrag = texture2D(frozenScene, texCoor);
    
    gl_FragColor = mix(newFrag, oldFrag, alpha);
}
