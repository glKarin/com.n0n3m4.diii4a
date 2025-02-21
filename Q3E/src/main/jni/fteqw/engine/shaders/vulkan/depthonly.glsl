//!!permu FRAMEBLEND
//!!permu SKELETAL

#include "sys/defs.h"

//standard shader used for drawing shadowmap depth.
//also used for masking off portals and other things that want depth and no colour.
//must support skeletal and 2-way vertex blending or Bad Things Will Happen.
//the vertex shader is responsible for calculating lighting values.

#ifdef VERTEX_SHADER
#include "sys/skeletal.h"
void main ()
{
	gl_Position = skeletaltransform();
}
#endif

#ifdef FRAGMENT_SHADER
void main ()
{
	//must always draw something, supposedly. It might as well be black.
	//in gl anyway, vulkan doesn't need it. gl_FragColor = vec4(0, 0, 0, 1);
}
#endif

