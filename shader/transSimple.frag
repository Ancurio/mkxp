
uniform sampler2D frozenScene;
uniform sampler2D currentScene;
uniform float prog;

void main()
{
    vec2 texCoor = gl_TexCoord[0].st;
    vec4 newPixel = texture2D(currentScene, texCoor);
    vec4 oldPixel = texture2D(frozenScene, texCoor);

    gl_FragColor = mix(oldPixel, newPixel, prog);
}
