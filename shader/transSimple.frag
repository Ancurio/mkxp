/* Fragment shader that produces a simple
 * fade in / fade out type transition */

uniform sampler2D frozenScene;
uniform sampler2D currentScene;
uniform float prog;

in vec2 v_texCoord;

out vec4 fragColor;

void main() {
  vec4 newPixel = texture(currentScene, v_texCoord);
  vec4 oldPixel = texture(frozenScene, v_texCoord);

  fragColor = mix(oldPixel, newPixel, prog);
}
