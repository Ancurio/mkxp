/* Fragment shader dealing with transitions */

uniform sampler2D currentScene;
uniform sampler2D frozenScene;
uniform sampler2D transMap;
/* Normalized */
uniform float prog;
/* Vague [0, 512] normalized */
uniform float vague;

in vec2 v_texCoord;

out vec4 fragColor;

void main() {
  float transV = texture(transMap, v_texCoord).r;
  float cTransV = clamp(transV, prog, prog + vague);
  lowp float alpha = (cTransV - prog) / vague;

  vec4 newFrag = texture(currentScene, v_texCoord);
  vec4 oldFrag = texture(frozenScene, v_texCoord);

  fragColor = mix(newFrag, oldFrag, alpha);
}
