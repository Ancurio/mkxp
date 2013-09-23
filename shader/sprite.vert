
uniform mat4 projMat;

uniform mat4 spriteMat;

uniform vec2 texSizeInv;

attribute vec2 position;
attribute vec2 texCoord;

varying vec2 v_texCoord;

void main()
{
	gl_Position = projMat * spriteMat * vec4(position, 0, 1);
	v_texCoord = texCoord * texSizeInv;
}
