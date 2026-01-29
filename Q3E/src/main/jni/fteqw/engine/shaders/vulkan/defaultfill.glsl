#include "sys/defs.h"

layout(location=0) varying vec4 vc;

#ifdef VERTEX_SHADER
void main ()
{
	vc = v_colour;
	gl_Position = ftetransform();
}
#endif

#ifdef FRAGMENT_SHADER
void main ()
{
	gl_FragColor = vc;
}
#endif
