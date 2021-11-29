
uniform mat4 projMat;

uniform mat4 spriteMat;

uniform vec2 texSizeInv;
uniform vec2 patternSizeInv;
uniform vec2 patternScroll;
uniform bool renderPattern;

attribute vec2 position;
attribute vec2 texCoord;

varying vec2 v_texCoord;
varying vec2 v_patCoord;

void main()
{
	gl_Position = projMat * spriteMat * vec4(position, 0, 1);
    
	v_texCoord = texCoord * texSizeInv;
    if (renderPattern) {
        v_patCoord = (texCoord * patternSizeInv) - (patternScroll * patternSizeInv);
    }
}
