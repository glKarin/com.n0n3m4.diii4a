!!samps 2
//pretty much a regular sky shader
//though in reality we should render a sun circle in the middle.
//still, its kinda cool to have scrolling clouds masking out parts of the sun.

#ifdef VERTEX_SHADER
varying vec3 pos;
void main ()
{
	pos = v_position.xyz;
	gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
uniform float e_time;
uniform vec3 e_eyepos;
varying vec3 pos;
void main ()
{
	vec2 tccoord;
	vec3 dir = pos - e_eyepos;
	dir.z *= 3.0;
	dir.xy /= 0.5*length(dir);
	tccoord = (dir.xy + e_time*0.03125);
	vec3 solid = vec3(texture2D(s_t0, tccoord));
	tccoord = (dir.xy + e_time*0.0625);
	vec4 clouds = texture2D(s_t1, tccoord);
	gl_FragColor.rgb = (solid.rgb*(1.0-clouds.a)) + (clouds.a*clouds.rgb);
}
#endif
