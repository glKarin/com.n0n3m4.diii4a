/*
	macros:
		_USING_DEPTH_TEXTURE: using depth texture instead RGBA texture.
		_PACK_FLOAT: pack float when using RGBA texture.
		_DEBUG: output z value to red component.
*/
#version 100
//#pragma optimize(off)

precision highp float;

#ifdef _PACK_FLOAT
vec4 pack (highp float depth)
{
	const highp vec4 bitSh = vec4(256.0 * 256.0 * 256.0, 256.0 * 256.0, 256.0, 1.0);
	const highp vec4 bitMsk = vec4(0.0, 1.0 / 256.0, 1.0 / 256.0, 1.0 / 256.0);
	highp vec4 comp = fract(depth * bitSh);
	comp -= comp.xxyz * bitMsk;
	return depth < 1.0 ? comp : vec4(1.0, 1.0, 1.0, 1.0);
}
#endif

void main(void)
{
#ifdef _USING_DEPTH_TEXTURE
   #ifdef _DEBUG
       gl_FragColor = vec4((gl_FragCoord.z + 1.0) * 0.5, 0.0, 0.0, 1.0); // DEBUG
   #endif
#else
	highp float depth;
	depth = gl_FragCoord.z;
   #ifdef _PACK_FLOAT
		gl_FragColor = gl_FragColor = pack(depth);
   #else
		gl_FragColor = vec4(depth, 0.0, 0.0, 1.0);
   #endif
#endif
}