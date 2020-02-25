/* Shader for approximating the way RMXP does bitmap
 * blending via DirectDraw */

uniform sampler2D source;
uniform sampler2D destination;

uniform vec4 subRect;

uniform lowp float opacity;

in vec2 v_texCoord;

out vec4 fragColor;

void main() {
  vec2 coor = v_texCoord;
  vec2 dstCoor = (coor - subRect.xy) * subRect.zw;

  vec4 srcFrag = texture(source, coor);
  vec4 dstFrag = texture(destination, dstCoor);

  vec4 resFrag;

  float co1 = srcFrag.a * opacity;
  float co2 = dstFrag.a * (1.0 - co1);
  resFrag.a = co1 + co2;

  if (resFrag.a == 0.0)
    resFrag.rgb = srcFrag.rgb;
  else
    resFrag.rgb = (co1 * srcFrag.rgb + co2 * dstFrag.rgb) / resFrag.a;

  fragColor = resFrag;
}
