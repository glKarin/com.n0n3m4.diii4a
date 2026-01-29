!!ver 100 300
!!permu TESS
!!permu BUMP
!!permu FRAMEBLEND
!!permu SKELETAL
!!permu UPPERLOWER
!!permu FOG
!!permu REFLECTCUBEMASK
!!cvarf r_glsl_offsetmapping_scale
!!cvardf r_glsl_pcf
!!cvardf r_tessellation_level=5
!!samps diffuse normalmap specular upper lower reflectcube reflectmask
!!samps =PCF shadowmap
!!samps =CUBE projectionmap

#if defined(ORM) || defined(SG)
	#define PBR
#endif

#include "sys/defs.h"

//this is the main shader responsible for realtime dlights.

//texture units:
//s0=diffuse, s1=normal, s2=specular, s3=shadowmap
//custom modifiers:
//PCF(shadowmap)
//CUBEPROJ(projected cubemap)
//SPOT(projected circle
//CUBESHADOW

#if 0 && defined(GL_ARB_texture_gather) && defined(PCF) 
#extension GL_ARB_texture_gather : enable
#endif

#ifdef UPPERLOWER
#define UPPER
#define LOWER
#endif

//if there's no vertex normals known, disable some stuff.
//FIXME: this results in dupe permutations.
#ifdef NOBUMP
#undef SPECULAR
#undef BUMP
#undef OFFSETMAPPING
#endif

#if !defined(TESS_CONTROL_SHADER)
	varying vec2 tcbase;
	varying vec3 lightvector;
	#if defined(VERTEXCOLOURS)
		varying vec4 vc;
	#endif
	#if defined(SPECULAR) || defined(OFFSETMAPPING) || defined(REFLECTCUBEMASK) || defined(PBR)
		varying vec3 eyevector;
	#endif
	#ifdef REFLECTCUBEMASK
		varying mat3 invsurface;
	#endif
	#if defined(PCF) || defined(CUBE) || defined(SPOT) || defined(ORTHO)
		varying vec4 vtexprojcoord;
	#endif
#endif


#ifdef VERTEX_SHADER
#ifdef TESS
varying vec3 vertex, normal;
#endif
#include "sys/skeletal.h"
void main ()
{
	vec3 n, s, t, w;
	gl_Position = skeletaltransform_wnst(w,n,s,t);
n = normalize(n);
s = normalize(s);
t = normalize(t);
	tcbase = v_texcoord;	//pass the texture coords straight through
#ifdef ORTHO
	vec3 lightminusvertex = -l_lightdirection;
	lightvector.x = dot(lightminusvertex, s.xyz);
	lightvector.y = dot(lightminusvertex, t.xyz);
	lightvector.z = dot(lightminusvertex, n.xyz);
#else
	vec3 lightminusvertex = l_lightposition - w.xyz;
	#ifdef NOBUMP
		//the only important thing is distance
		lightvector = lightminusvertex;
	#else
		//the light direction relative to the surface normal, for bumpmapping.
		lightvector.x = dot(lightminusvertex, s.xyz);
		lightvector.y = dot(lightminusvertex, t.xyz);
		lightvector.z = dot(lightminusvertex, n.xyz);
	#endif
#endif
#if defined(VERTEXCOLOURS)
	vc = v_colour;
#endif
#if defined(SPECULAR)||defined(OFFSETMAPPING) || defined(REFLECTCUBEMASK) || defined(PBR)
	vec3 eyeminusvertex = e_eyepos - w.xyz;
	eyevector.x = dot(eyeminusvertex, s.xyz);
	eyevector.y = dot(eyeminusvertex, t.xyz);
	eyevector.z = dot(eyeminusvertex, n.xyz);
#endif
#ifdef REFLECTCUBEMASK
	invsurface = mat3(v_svector, v_tvector, v_normal);
#endif
#if defined(PCF) || defined(SPOT) || defined(CUBE) || defined(ORTHO)
	//for texture projections/shadowmapping on dlights
	vtexprojcoord = (l_cubematrix*vec4(w.xyz, 1.0));
#endif

#ifdef TESS
	vertex = w;
	normal = n;
#endif
}
#endif






#if defined(TESS_CONTROL_SHADER)
layout(vertices = 3) out;

in vec3 vertex[];
out vec3 t_vertex[];
in vec3 normal[];
out vec3 t_normal[];
in vec2 tcbase[];
out vec2 t_tcbase[];
in vec3 lightvector[];
out vec3 t_lightvector[];
#if defined(VERTEXCOLOURS)
in vec4 vc[];
out vec4 t_vc[];
#endif
#if defined(SPECULAR) || defined(OFFSETMAPPING) || defined(REFLECTCUBEMASK) || defined(PBR)
in vec3 eyevector[];
out vec3 t_eyevector[];
#endif
void main()
{
	//the control shader needs to pass stuff through
#define id gl_InvocationID
	t_vertex[id] = vertex[id];
	t_normal[id] = normal[id];
	t_tcbase[id] = tcbase[id];
	t_lightvector[id] = lightvector[id];
#if defined(VERTEXCOLOURS)
	t_vc[id] = vc[id];
#endif
#if defined(SPECULAR) || defined(OFFSETMAPPING) || defined(REFLECTCUBEMASK) || defined(PBR)
	t_eyevector[id] = eyevector[id];
#endif

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
in vec2 t_tcbase[];
in vec3 t_lightvector[];
#if defined(VERTEXCOLOURS)
in vec4 t_vc[];
#endif
#if defined(SPECULAR) || defined(OFFSETMAPPING) || defined(REFLECTCUBEMASK) || defined(PBR)
in vec3 t_eyevector[];
#endif

#define LERP(a) (gl_TessCoord.x*a[0] + gl_TessCoord.y*a[1] + gl_TessCoord.z*a[2])
void main()
{
#define factor 1.0
	tcbase = LERP(t_tcbase);
	vec3 w = LERP(t_vertex);

	vec3 t0 = w - dot(w-t_vertex[0],t_normal[0])*t_normal[0];
	vec3 t1 = w - dot(w-t_vertex[1],t_normal[1])*t_normal[1];
	vec3 t2 = w - dot(w-t_vertex[2],t_normal[2])*t_normal[2];
	w = w*(1.0-factor) + factor*(gl_TessCoord.x*t0+gl_TessCoord.y*t1+gl_TessCoord.z*t2);

#if defined(PCF) || defined(SPOT) || defined(CUBE) || defined(ORTHO)
	//for texture projections/shadowmapping on dlights
	vtexprojcoord = (l_cubematrix*vec4(w.xyz, 1.0));
#endif

	//FIXME: we should be recalcing these here, instead of just lerping them
	lightvector = LERP(t_lightvector);
#if defined(VERTEXCOLOURS)
	vc = LERP(t_vc);
#endif
#if defined(SPECULAR) || defined(OFFSETMAPPING) || defined(REFLECTCUBEMASK) || defined(PBR)
	eyevector = LERP(t_eyevector);
#endif

	gl_Position = m_modelviewprojection * vec4(w,1.0);
}
#endif











#ifdef FRAGMENT_SHADER

#include "sys/fog.h"
#include "sys/pcf.h"
#ifdef OFFSETMAPPING
#include "sys/offsetmapping.h"
#endif

#include "sys/pbr.h"

void main ()
{
#ifdef ORTHO
	float colorscale = 1.0;
#else
	float colorscale = max(1.0 - (dot(lightvector, lightvector)/(l_lightradius*l_lightradius)), 0.0);
#endif
#ifdef PCF
	/*filter the light by the shadowmap. logically a boolean, but we allow fractions for softer shadows*/
	colorscale *= ShadowmapFilter(s_shadowmap, vtexprojcoord);
#endif
#if defined(SPOT)
	/*filter the colour by the spotlight. discard anything behind the light so we don't get a mirror image*/
	if (vtexprojcoord.w < 0.0) discard;
	vec2 spot = ((vtexprojcoord.st)/vtexprojcoord.w);
	colorscale*=1.0-(dot(spot,spot));
#endif

//read raw texture samples (offsetmapping munges the tex coords first)
#ifdef OFFSETMAPPING
	vec2 tcoffsetmap = offsetmap(s_normalmap, tcbase, eyevector);
#define tcbase tcoffsetmap
#endif
#if defined(FLAT)
	vec4 bases = vec4(FLAT, FLAT, FLAT, 1.0);
#else
	vec4 bases = texture2D(s_diffuse, tcbase);
	#ifdef VERTEXCOLOURS
		bases.rgb *= bases.a;
	#endif
#endif
#ifdef UPPER
	vec4 uc = texture2D(s_upper, tcbase);
	bases.rgb += uc.rgb*e_uppercolour*uc.a;
#endif
#ifdef LOWER
	vec4 lc = texture2D(s_lower, tcbase);
	bases.rgb += lc.rgb*e_lowercolour*lc.a;
#endif
#if defined(BUMP) || defined(SPECULAR) || defined(REFLECTCUBEMASK) || defined(PBR)
	vec3 bumps = normalize(vec3(texture2D(s_normalmap, tcbase)) - 0.5);
#elif defined(REFLECTCUBEMASK)
	vec3 bumps = vec3(0.0,0.0,1.0);
#endif
#ifdef SPECULAR
	vec4 specs = texture2D(s_specular, tcbase);
#endif

	#define dielectricSpecular 0.04
	#ifdef SPECULAR
		#ifdef ORM	//pbr-style occlusion+roughness+metalness
			#define occlusion specs.r
			#define roughness clamp(specs.g, 0.04, 1.0)
			#define metalness specs.b
			#define gloss 1.0 //sqrt(1.0-roughness)
			#define ambientrgb (specrgb+col.rgb)
			vec3 specrgb = mix(vec3(dielectricSpecular), bases.rgb, metalness);
			bases.rgb = bases.rgb * (1.0 - dielectricSpecular) * (1.0-metalness);
		#elif defined(SG) //pbr-style specular+glossiness
			//occlusion needs to be baked in. :(
			#define roughness (1.0-specs.a)
			#define gloss specs.a
			#define specrgb specs.rgb
			#define ambientrgb (specs.rgb+col.rgb)
		#else   //blinn-phong
			#define roughness (1.0-specs.a)
			#define gloss specs.a
			#define specrgb specs.rgb
			#define ambientrgb col.rgb
		#endif
	#else
		#define roughness 0.3
		#define specrgb bases.rgb //vec3(dielectricSpecular)
	#endif

#ifdef PBR
	vec3 diff = DoPBR(bumps, normalize(eyevector), normalize(lightvector), roughness, bases.rgb, specrgb, l_lightcolourscale);
#else
	vec3 diff;
	#ifdef NOBUMP
		//surface can only support ambient lighting, even for lights that try to avoid it.
		diff = bases.rgb * (l_lightcolourscale.x+l_lightcolourscale.y);
	#else
		vec3 nl = normalize(lightvector);
		#ifdef BUMP
			diff = bases.rgb * (l_lightcolourscale.x + l_lightcolourscale.y * max(dot(bumps, nl), 0.0));
		#else
			//we still do bumpmapping even without bumps to ensure colours are always sane. light.exe does it too.
			diff = bases.rgb * (l_lightcolourscale.x + l_lightcolourscale.y * max(dot(vec3(0.0, 0.0, 1.0), nl), 0.0));
		#endif
	#endif
	#ifdef SPECULAR
		vec3 halfdir = normalize(normalize(eyevector) + nl);
		float spec = pow(max(dot(halfdir, bumps), 0.0), FTE_SPECULAR_EXPONENT * gloss)*float(SPECMUL);
		diff += l_lightcolourscale.z * spec * specrgb;
	#endif
#endif

#ifdef REFLECTCUBEMASK
	vec3 rtc = reflect(-eyevector, bumps);
	rtc = rtc.x*invsurface[0] + rtc.y*invsurface[1] + rtc.z*invsurface[2];
	rtc = (m_model * vec4(rtc.xyz,0.0)).xyz;
	diff += texture2D(s_reflectmask, tcbase).rgb * textureCube(s_reflectcube, rtc).rgb;
#endif

#ifdef CUBE
	/*filter the colour by the cubemap projection*/
	diff *= textureCube(s_projectionmap, vtexprojcoord.xyz).rgb;
#endif

#if defined(PROJECTION)
	/*2d projection, not used*/
//	diff *= texture2d(s_projectionmap, shadowcoord);
#endif
#if defined(occlusion) && !defined(NOOCCLUDE)
	diff *= occlusion;
#endif
#if defined(VERTEXCOLOURS)
	diff *= vc.rgb * vc.a;
#endif

	diff *= colorscale*l_lightcolour;
	gl_FragColor = vec4(fog3additive(diff), 1.0);
}
#endif

