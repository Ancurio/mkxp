
uniform mat4 projMat;

uniform vec2 texSizeInv;
uniform vec2 translation;

uniform float aniIndex;

attribute vec2 position;
attribute vec2 texCoord;

varying vec2 v_texCoord;

const float atAreaW = 96.0;
const float atAreaH = 128.0*7.0;
const float atAniOffset = 32.0*3.0;

void main()
{
	vec2 tex = texCoord;

	lowp float pred = float(tex.x <= atAreaW && tex.y <= atAreaH);
	tex.x += aniIndex * atAniOffset * pred;

	gl_Position = projMat * vec4(position + translation, 0, 1);

	v_texCoord = tex * texSizeInv;
}
