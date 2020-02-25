
uniform lowp float alpha;

in lowp vec4 v_color;

out vec4 fragColor;
void main() { fragColor = vec4(v_color.rgb * alpha, 1); }
