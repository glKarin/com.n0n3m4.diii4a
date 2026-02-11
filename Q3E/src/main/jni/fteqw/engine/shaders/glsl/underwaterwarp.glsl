!!cvarf r_waterwarp
!!samps screen=0 warp=1 edge=2

//this is a post processing shader that is drawn fullscreen whenever the view is underwater.
//its generally expected to warp the view a little.

varying vec2 v_stc;
varying vec2 v_warp;
varying vec2 v_edge;
#ifdef VERTEX_SHADER
attribute vec2 v_texcoord;
uniform float e_time;
void main ()
{
	gl_Position = ftetransform();
	v_stc = vec2(v_texcoord.x, 1.0-v_texcoord.y);
	v_warp.s = e_time * 0.25 + v_texcoord.s;
	v_warp.t = e_time * 0.25 + v_texcoord.t;
	v_edge = v_texcoord.xy;
}
#endif
#ifdef FRAGMENT_SHADER
uniform vec4 e_rendertexturescale;
uniform float cvar_r_waterwarp;
void main ()
{
	vec2 amp		= (0.010 / 0.625) * cvar_r_waterwarp * texture2D(s_edge, v_edge).rg;
	vec3 offset	= (texture2D(s_warp, v_warp).rgb - 0.5) * 2.0;
	vec2 temp		= v_stc + offset.xy * amp;
	gl_FragColor	= texture2D(s_screen, temp*e_rendertexturescale.st);
}
#endif
