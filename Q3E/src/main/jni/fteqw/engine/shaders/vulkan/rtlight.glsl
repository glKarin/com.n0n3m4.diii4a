!!samps diffuse normalmap specular shadowmap upper lower reflectmask reflectcube projectionmap
!!cvarf r_glsl_offsetmapping=0
!!cvarf gl_specular=0
!!cvarf r_glsl_offsetmapping_scale=0.04
!!cvari r_glsl_pcf=5
!!permu BUMP
!!permu UPPERLOWER
!!permu REFLECTCUBEMASK
!!argb pcf=0
!!argb spot=0
!!argb cube=0
!!permu FOG
!!cvarb r_fog_exp2=true
!!cvarb r_fog_linear=false
//!!permu FRAMEBLEND
//!!permu SKELETAL

#include "sys/defs.h"
//const bool UPPERLOWER = false;
//const bool REFLECTCUBEMASK = false;
//const bool BUMP = true;
//const float cvar_gl_specular = 1.0;
//const int cvar_r_glsl_pcf = 5;
//const float cvar_r_glsl_offsetmapping = 0.0;
//const float cvar_r_glsl_offsetmapping_scale = 0.04;
//layout(constant_id=1) const bool arg_pcf = true;
//layout(constant_id=2) const bool arg_spot = false;
//layout(constant_id=3) const bool arg_cube = false;

#define USE_ARB_SHADOW

#ifndef USE_ARB_SHADOW
//fall back on regular samplers if we must
#define sampler2DShadow sampler2D
#else
#define shadow2D texture
#endif

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

//if there's no vertex normals known, disable some stuff.
//FIXME: this results in dupe permutations.
#ifdef NOBUMP
#undef SPECULAR
#undef BUMP
#undef OFFSETMAPPING
#endif


layout(location = 0) varying vec2 tc;
layout(location = 1) varying vec3 lightvector;
layout(location = 2) varying vec3 eyevector;
layout(location = 3) varying vec4 vtexprojcoord;
layout(location = 4) varying mat3 invsurface;
#ifdef RAY_QUERY
layout(location = 7) varying vec3 wcoord;
#endif


#ifdef VERTEX_SHADER
#include "sys/skeletal.h"
void main ()
{
	vec3 n, s, t, w;
	gl_Position = skeletaltransform_wnst(w,n,s,t);
	tc = v_texcoord;	//pass the texture coords straight through
	vec3 lightminusvertex = (m_modelinv*vec4(l_lightposition,1.0)).xyz - w.xyz;
	if (true)//BUMP || SPECULAR)
	{
		//the light direction relative to the surface normal, for bumpmapping.
		lightvector.x = dot(lightminusvertex, s.xyz);
		lightvector.y = dot(lightminusvertex, t.xyz);
		lightvector.z = dot(lightminusvertex, n.xyz);
	}
	else
		lightvector = lightminusvertex;
	if (SPECULAR || OFFSETMAPPING || REFLECTCUBEMASK)
	{
		vec3 eyeminusvertex = e_eyepos - w.xyz;
		eyevector.x = dot(eyeminusvertex, s.xyz);
		eyevector.y = dot(eyeminusvertex, t.xyz);
		eyevector.z = dot(eyeminusvertex, n.xyz);
	}
	if (REFLECTCUBEMASK)
	{
		invsurface[0] = v_svector;
		invsurface[1] = v_tvector;
		invsurface[2] = v_normal;
	}

	if (arg_pcf || arg_spot || arg_cube)
	{
		//for texture projections/shadowmapping on dlights
		vtexprojcoord = l_cubematrix*m_model*vec4(w.xyz, 1.0);
	}
#ifdef RAY_QUERY
	wcoord = vec3(m_model*vec4(w+n*0.1, 1.0));	//push it half a qu away from the face, so we're less likely to get precision errors in the rays.
#endif
}
#endif




#ifdef FRAGMENT_SHADER
#include "sys/fog.h"

#ifdef RAY_QUERY
float RayQueryFilter(void)
{
	rayQueryEXT rq;
//FIXME: no ortho
#define l_origin e_eyepos
	rayQueryInitializeEXT(rq, toplevelaccel, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, l_lightposition, 0.0, wcoord-l_lightposition, 1.0);
	rayQueryProceedEXT(rq);
//TODO: filter it through blended stuff, and alpha-tested stuff.
	return float(rayQueryGetIntersectionTypeEXT(rq, true) == gl_RayQueryCommittedIntersectionNoneEXT);
}
#else
//uniform vec4 l_shadowmapproj; //light projection matrix info
//uniform vec2 l_shadowmapscale;	//xy are the texture scale, z is 1, w is the scale.
vec3 ShadowmapCoord(void)
{
	if (arg_spot)
	{
		//bias it. don't bother figuring out which side or anything, its not needed
		//l_projmatrix contains the light's projection matrix so no other magic needed
		return ((vtexprojcoord.yxz-vec3(0.0,0.0,0.015))/vtexprojcoord.w + vec3(1.0, -1.0, 1.0)) * vec3(0.5, -0.5, 0.5);
	}
//	else if (CUBESHADOW)
//	{
//		vec3 shadowcoord = vshadowcoord.xyz / vshadowcoord.w;
//		#define dosamp(x,y) shadowCube(s_shadowmap, shadowcoord + vec2(x,y)*texscale.xy).r
//	}
	//figure out which axis to use
	//texture is arranged thusly:
	//forward left  up
	//back    right down
	vec3 dir = abs(vtexprojcoord.xyz);
	//assume z is the major axis (ie: forward from the light)
	vec3 t = vtexprojcoord.xyz;
	float ma = dir.z;
	vec3 axis = vec3(0.5/3.0, 0.5/2.0, 0.5);
	if (dir.x > ma)
	{
		ma = dir.x;
		t = vtexprojcoord.zyx;
		axis.x = 0.5;
	}
	if (dir.y > ma)
	{
		ma = dir.y;
		t = vtexprojcoord.xzy;
		axis.x = 2.5/3.0;
	}
	//if the axis is negative, flip it.
	if (t.z > 0.0)
	{
		axis.y = 1.5/2.0;
		t.z = -t.z;
	}

	//we also need to pass the result through the light's projection matrix too
	//the 'matrix' we need only contains 5 actual values. and one of them is a -1. So we might as well just use a vec4.
	//note: the projection matrix also includes scalers to pinch the image inwards to avoid sampling over borders, as well as to cope with non-square source image
	//the resulting z is prescaled to result in a value between -0.5 and 0.5.
	//also make sure we're in the right quadrant type thing
	return axis + ((l_shadowmapproj.xyz*t.xyz + vec3(0.0, 0.0, l_shadowmapproj.w)) / -t.z);
}

float ShadowmapFilter(void)
{
	vec3 shadowcoord = ShadowmapCoord();

	#if 0//def GL_ARB_texture_gather
		vec2 ipart, fpart;
		#define dosamp(x,y) textureGatherOffset(s_shadowmap, ipart.xy, vec2(x,y)))
		vec4 tl = step(shadowcoord.z, dosamp(-1.0, -1.0));
		vec4 bl = step(shadowcoord.z, dosamp(-1.0, 1.0));
		vec4 tr = step(shadowcoord.z, dosamp(1.0, -1.0));
		vec4 br = step(shadowcoord.z, dosamp(1.0, 1.0));
		//we now have 4*4 results, woo
		//we can just average them for 1/16th precision, but that's still limited graduations
		//the middle four pixels are 'full strength', but we interpolate the sides to effectively give 3*3
		vec4 col =     vec4(tl.ba, tr.ba) + vec4(bl.rg, br.rg) + //middle two rows are full strength
				mix(vec4(tl.rg, tr.rg), vec4(bl.ba, br.ba), fpart.y); //top+bottom rows
		return dot(mix(col.rgb, col.agb, fpart.x), vec3(1.0/9.0));	//blend r+a, gb are mixed because its pretty much free and gives a nicer dot instruction instead of lots of adds.

	#else
#ifdef USE_ARB_SHADOW
		//with arb_shadow, we can benefit from hardware acclerated pcf, for smoother shadows
		#define dosamp(x,y) shadow2D(s_shadowmap, shadowcoord.xyz + (vec3(x,y,0.0)*l_shadowmapscale.xyx)).r
#else
		//this will probably be a bit blocky.
		#define dosamp(x,y) float(texture2D(s_shadowmap, shadowcoord.xy + (vec2(x,y)*l_shadowmapscale.xy)).r >= shadowcoord.z)
#endif
		float s = 0.0;
		if (cvar_r_glsl_pcf < 5)
		{
			s += dosamp(0.0, 0.0);
			return s;
		}
		else if (cvar_r_glsl_pcf < 9)
		{
			s += dosamp(-1.0, 0.0);
			s += dosamp(0.0, -1.0);
			s += dosamp(0.0, 0.0);
			s += dosamp(0.0, 1.0);
			s += dosamp(1.0, 0.0);
			return s/5.0;
		}
		else
		{
			s += dosamp(-1.0, -1.0);
			s += dosamp(-1.0, 0.0);
			s += dosamp(-1.0, 1.0);
			s += dosamp(0.0, -1.0);
			s += dosamp(0.0, 0.0);
			s += dosamp(0.0, 1.0);
			s += dosamp(1.0, -1.0);
			s += dosamp(1.0, 0.0);
			s += dosamp(1.0, 1.0);
			return s/9.0;
}
	#endif
}
#endif


#include "sys/offsetmapping.h"

void main ()
{
//read raw texture samples (offsetmapping munges the tex coords first)
	vec2 tcbase;
	if (OFFSETMAPPING)
		tcbase = offsetmap(s_normalmap, tc, eyevector);
	else
		tcbase = tc;
#if defined(FLAT)
	vec3 bases = vec3(1.0);
#else
	vec3 bases = vec3(texture2D(s_diffuse, tcbase));
#endif
	if (UPPERLOWER)
	{
		vec4 uc = texture2D(s_upper, tcbase);
		bases.rgb += uc.rgb*e_uppercolour*uc.a;

		vec4 lc = texture2D(s_lower, tcbase);
		bases.rgb += lc.rgb*e_lowercolour*lc.a;
	}
	vec3 bumps;
	if (BUMP)
		bumps = normalize(vec3(texture2D(s_normalmap, tcbase)) - 0.5);
	else
		bumps = vec3(0.0,0.0,1.0);

	float colorscale = max(1.0 - (dot(lightvector, lightvector)/(l_lightradius*l_lightradius)), 0.0);
	vec3 diff;
#ifdef NOBUMP
	//surface can only support ambient lighting, even for lights that try to avoid it.
	diff = bases * (l_lightcolourscale.x+l_lightcolourscale.y);
#else
	vec3 nl = normalize(lightvector);
	if (BUMP)
		diff = bases * (l_lightcolourscale.x + l_lightcolourscale.y * max(dot(bumps, nl), 0.0));
	else
	{
		//we still do bumpmapping even without bumps to ensure colours are always sane. light.exe does it too.
		diff = bases * (l_lightcolourscale.x + l_lightcolourscale.y * max(dot(vec3(0.0, 0.0, 1.0), nl), 0.0));
	}
#endif


	if (SPECULAR)
	{
		vec4 specs = texture2D(s_specular, tcbase);
		vec3 halfdir = normalize(normalize(eyevector) + nl);
		float spec = pow(max(dot(halfdir, bumps), 0.0), 32.0 * specs.a);
		diff += l_lightcolourscale.z * spec * specs.rgb;
	}

	if (REFLECTCUBEMASK)
	{
		vec3 rtc = reflect(-eyevector, bumps);
		rtc = rtc.x*invsurface[0] + rtc.y*invsurface[1] + rtc.z*invsurface[2];
		rtc = (m_model * vec4(rtc.xyz,0.0)).xyz;
		diff += texture2D(s_reflectmask, tcbase).rgb * textureCube(s_reflectcube, rtc).rgb;
	}

	if (arg_cube)
	{
		/*filter the colour by the cubemap projection*/
		diff *= textureCube(s_projectionmap, vtexprojcoord.xyz).rgb;
	}

	if (arg_spot)
	{
		/*filter the colour by the spotlight. discard anything behind the light so we don't get a mirror image*/
		if (vtexprojcoord.w < 0.0) discard;
		vec2 spot = ((vtexprojcoord.st)/vtexprojcoord.w);
		colorscale*=1.0-(dot(spot,spot));
	}

#ifdef RAY_QUERY
	colorscale *= RayQueryFilter();
#else
	if (arg_pcf)
	{
		/*filter the light by the shadowmap. logically a boolean, but we allow fractions for softer shadows*/
//diff.rgb = (vtexprojcoord.xyz/vtexprojcoord.w) * 0.5 + 0.5;
		colorscale *= ShadowmapFilter();
//		diff = ShadowmapCoord();
	}
#endif

#if defined(PROJECTION)
	/*2d projection, not used*/
//	diff *= texture2d(s_projectionmap, shadowcoord);
#endif

	gl_FragColor.rgb = fog3additive(diff*colorscale*l_lightcolour);

	gl_FragColor.a = 1.0;
}
#endif

