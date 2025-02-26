!!cvardf r_glsl_ascii_mono=0
!!samps screen=0

//derived from https://www.shadertoy.com/view/lssGDj

#include "sys/defs.h"
varying vec2 texcoord;

#ifdef VERTEX_SHADER
void main()
{
	texcoord = v_texcoord.xy;
	texcoord.y = 1.0 - texcoord.y;
	gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
uniform vec2 e_sourcesize;

float character(float n, vec2 p)
{
	p = floor(p*vec2(4.0, -4.0) + 2.5);
	if (clamp(p.x, 0.0, 4.0) == p.x && clamp(p.y, 0.0, 4.0) == p.y)
	{
		if (int(mod(n/exp2(p.x + 5.0*p.y), 2.0)) == 1) return 1.0;
	}	
	return 0.0;
}

void main(void)
{
	vec2 uv = floor(texcoord.xy * e_sourcesize); //in pixels.
	vec3 col = texture2D(s_screen, (floor(uv/8.0)*8.0+4.0)/e_sourcesize.xy).rgb;	

	float gray = 0.3 * col.r + 0.59 * col.g + 0.11 * col.b;

	if (float(r_glsl_ascii_mono) != 0.0)
		gray = gray = pow(gray, 0.7);	//quake is just too dark otherwise.
	else
		gray = gray = pow(gray, 0.45);	//col*char is FAR too dark otherwise, and much of the colour will come from the col term anyway.

	float n =  0.0;              // space
	if (gray > 0.1) n = 4096.0;	// .
	if (gray > 0.2) n = 65600.0;    // :
	if (gray > 0.3) n = 332772.0;   // *
	if (gray > 0.4) n = 15255086.0; // o 
	if (gray > 0.5) n = 23385164.0; // &
	if (gray > 0.6) n = 15252014.0; // 8
	if (gray > 0.7) n = 13199452.0; // @
	if (gray > 0.8) n = 11512810.0; // #

	vec2 p = mod(uv/4.0, 2.0) - vec2(1.0);
	if (float(r_glsl_ascii_mono) != 0.0)
		col = vec3(character(n, p));
	else
		col = col*character(n, p);	//note that this is kinda cheating.
	gl_FragColor = vec4(col, 1.0);
}
#endif
