
uniform mat4 projMat;

uniform mat4 spriteMat;

uniform vec2 texSizeInv;
uniform vec2 patternSizeInv;
uniform vec2 patternScroll;
uniform vec2 patternZoom;
uniform bool renderPattern;
uniform bool patternTile;

attribute vec2 position;
attribute vec2 texCoord;

varying vec2 v_texCoord;
varying vec2 v_patCoord;

void main()
{
	gl_Position = projMat * spriteMat * vec4(position, 0, 1);
    
    v_texCoord = texCoord * texSizeInv;
    
    if (renderPattern) {
        if (patternTile) {
            vec2 scroll = patternScroll * (patternSizeInv / texSizeInv);
            v_patCoord = (texCoord * (patternSizeInv / patternZoom)) - (scroll * patternSizeInv);
        }
        else {
            vec2 scroll = patternScroll * (patternSizeInv / texSizeInv);
            v_patCoord = (texCoord * (texSizeInv / patternZoom)) - (scroll * texSizeInv);
        }
    }
}
