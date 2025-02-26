!!samps 2
//this glsl shader is useful for cubemapped post processing effects (see csaddon for an example)
varying vec4 tf;
#ifdef VERTEX_SHADER
void main ()
{
	gl_Position = tf = vec4(v_position.xy,-1.0, 1.0);
}
#endif
#ifdef FRAGMENT_SHADER
void main()
{
	vec2 fc;
	fc = tf.xy / tf.w;
	vec3 raw = texture2D(s_t0, (1.0 + fc) / 2.0).rgb;
#define LUTSIZE 16.0
	vec3 scale = vec3((LUTSIZE-1.0)/LUTSIZE);
	vec3 bias = vec3(1.0/(2.0*LUTSIZE));
	gl_FragColor = texture3D(s_t1, raw * scale + bias);
}
#endif
