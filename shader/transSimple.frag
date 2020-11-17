/* Fragment shader that produces a simple
 * fade in / fade out type transition */

uniform sampler2D frozenScene;
uniform sampler2D currentScene;
uniform float prog;

varying vec2 v_texCoord;

void main()
{
    vec4 newPixel = texture2D(currentScene, v_texCoord);
    vec4 oldPixel = texture2D(frozenScene, v_texCoord);

    gl_FragColor = mix(oldPixel, newPixel, prog);
}
