
uniform mat4 projMat;

uniform vec2 texSizeInv;

attribute vec2 position;
attribute vec2 texCoord;

varying vec2 v_texCoord;
varying vec2 v_blurCoord[2];

void main()
{
	gl_Position = projMat * vec4(position, 0, 1);

	v_texCoord = texCoord * texSizeInv;
	v_blurCoord[0] = vec2(texCoord.x, texCoord.y-1.0) * texSizeInv;
	v_blurCoord[1] = vec2(texCoord.x, texCoord.y+1.0) * texSizeInv;
}
