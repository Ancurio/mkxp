// From https://raw.githubusercontent.com/Sentmoraap/doing-sdl-right/93a52a0db0ff2da5066cce12f5b9a2ac62e6f401/assets/lanczos3.frag
// Copyright 2020 Lilian Gimenez (Sentmoraap).
// MIT license.

uniform sampler2D texture;
uniform vec2 sourceSize;
uniform vec2 texSizeInv;
varying vec2 v_texCoord;

float lanczos3(float x)
{
	x = max(abs(x), 0.00001);
	float val = x * 3.141592654;
	return sin(val) * sin(val / 3) / (val * val);
}

void main()
{
	vec2 pixel = v_texCoord * sourceSize + 0.5;
	vec2 frac = fract(pixel);
	vec2 onePixel = texSizeInv;
	pixel = floor(pixel) * texSizeInv - onePixel / 2;

	float lanczosX[6];
	float sum = 0;
	for(int x = 0; x < 6; x++)
	{
		lanczosX[x] = lanczos3(x - 2 - frac.x);
		sum += lanczosX[x];
	}
	for(int x = 0; x < 6; x++) lanczosX[x] /= sum;
	sum = 0;
	float lanczosY[6];
	for(int y = 0; y < 6; y++)
	{
		lanczosY[y] = lanczos3(y - 2 - frac.y);
		sum += lanczosY[y];
	}
	for(int y = 0; y < 6; y++) lanczosY[y] /= sum;
	gl_FragColor = vec4(0);
	for(int y = -2; y <= 3; y++)
	{
		vec4 colour = vec4(0);
		for(int x = -2; x <= 3; x++)
				colour += texture2D(texture, pixel + vec2(x * onePixel.x, y * onePixel.y)).rgba * lanczosX[x + 2];
		gl_FragColor += colour * lanczosY[y + 2];
	}
}
