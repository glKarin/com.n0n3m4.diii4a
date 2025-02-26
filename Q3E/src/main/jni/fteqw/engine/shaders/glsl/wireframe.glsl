!!ver 100 150
!!permu TESS
!!permu FRAMEBLEND
!!permu SKELETAL
!!permu FOG
!!cvardf r_tessellation_level=5

#include "sys/defs.h"

//standard shader used for wireframe stuff.
//must support skeletal and 2-way vertex blending or Bad Things Will Happen.








#ifdef VERTEX_SHADER
#include "sys/skeletal.h"

#ifdef TESS
varying vec3 vertex;
varying vec3 normal;
#endif

void main ()
{
	vec3 n, s, t, w;
	gl_Position = skeletaltransform_wnst(w,n,s,t);

#ifdef TESS
	normal = n;
	vertex = w;
#endif
}
#endif










#if defined(TESS_CONTROL_SHADER)
layout(vertices = 3) out;

in vec3 vertex[];
out vec3 t_vertex[];
in vec3 normal[];
out vec3 t_normal[];
void main()
{
	//the control shader needs to pass stuff through
#define id gl_InvocationID
	t_vertex[id] = vertex[id];
	t_normal[id] = normal[id];

	gl_TessLevelOuter[0] = float(r_tessellation_level);
	gl_TessLevelOuter[1] = float(r_tessellation_level);
	gl_TessLevelOuter[2] = float(r_tessellation_level);
	gl_TessLevelInner[0] = float(r_tessellation_level);
}
#endif









#if defined(TESS_EVALUATION_SHADER)
layout(triangles) in;

in vec3 t_vertex[];
in vec3 t_normal[];

#define LERP(a) (gl_TessCoord.x*a[0] + gl_TessCoord.y*a[1] + gl_TessCoord.z*a[2])
void main()
{
#define factor 1.0
	vec3 w = LERP(t_vertex);

	vec3 t0 = w - dot(w-t_vertex[0],t_normal[0])*t_normal[0];
	vec3 t1 = w - dot(w-t_vertex[1],t_normal[1])*t_normal[1];
	vec3 t2 = w - dot(w-t_vertex[2],t_normal[2])*t_normal[2];
	w = w*(1.0-factor) + factor*(gl_TessCoord.x*t0+gl_TessCoord.y*t1+gl_TessCoord.z*t2);


	gl_Position = m_modelviewprojection * vec4(w,1.0);
}
#endif










#ifdef FRAGMENT_SHADER

#include "sys/fog.h"

void main ()
{
	gl_FragColor = fog4(vec4(1.0) * e_colourident);
}
#endif

