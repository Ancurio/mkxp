
uniform mat4 projMat;
attribute vec2 position;

void main() { gl_Position = projMat * vec4(position, 0, 1); }
