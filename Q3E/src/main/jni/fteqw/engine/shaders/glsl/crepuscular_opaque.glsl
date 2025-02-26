//opaque surfaces are drawn to the render target to mask out skies
#ifdef VERTEX_SHADER
void main ()
{
	gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
void main()
{
	gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
#endif
