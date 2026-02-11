!!ver 100 150
!!permu TESS
!!permu FULLBRIGHT
!!permu UPPERLOWER
!!permu FRAMEBLEND
!!permu SKELETAL
!!permu FOG
!!permu BUMP
!!permu REFLECTCUBEMASK
!!cvarf r_glsl_offsetmapping_scale
!!cvarf gl_specular
!!cvardf gl_affinemodels=0
!!cvardf r_tessellation_level=5
!!samps !EIGHTBIT diffuse normalmap specular fullbright upper lower reflectmask reflectcube
!!samps =EIGHTBIT paletted 1
!!samps =OCCLUDE occlusion
!!samps =USE_TRANSMISSION transmission	//only .r valid, multiplier for factor_transmission
!!samps =USE_VOLUME thickness			//only .g valid, multiplier for factor_volume_thickness, combined with factor_volume_rgb+factor_volume_distance(average distance travelled in metres)
//!!permu VC			// adds rgba vertex colour multipliers
//!!permu SPECULAR		// auto-added when gl_specular>0
//!!permu OFFSETMAPPING	// auto-added when r_glsl_offsetmapping is set
//!!permu NONORMALS		// states that there's no normals available, which affects lighting.
//!!permu ORM			// specularmap is r:Occlusion, g:Roughness, b:Metalness
//!!permu SG			// specularmap is rgb:F0, a:Roughness (instead of exponent)
//!!permu PBR			// an attempt at pbr logic (enabled from ORM or SG)
//!!permu NOOCCLUDE		// ignores the use of ORM's occlusion... yeah, stupid.
//!!permu OCCLUDE		// use an explicit occlusion texturemap (separate from roughness+metalness).
//!!permu EIGHTBIT		// uses software-style paletted colourmap lookups
//!!permu ALPHATEST		// if defined, this is the required alpha level (more versatile than doing it at the q3shader level)

#include "sys/defs.h"

//standard shader used for models.
//must support skeletal and 2-way vertex blending or Bad Things Will Happen.
//the vertex shader is responsible for calculating lighting values.

#if gl_affinemodels==1 && __VERSION__ >= 130 && !defined(GL_ES)
#define affine noperspective
#else
#define affine
#endif

#if defined(ORM) || defined(SG)
	#define PBR
#endif

#ifdef NONORMALS	//lots of things need normals to work properly. make sure nothing breaks simply because they added an extra texture.
	#undef BUMP
	#undef SPECULAR
	#undef OFFSETMAPPING
	#undef REFLECTCUBEMASK
#endif




#ifdef VERTEX_SHADER
#include "sys/skeletal.h"

affine varying vec2 tc;
varying vec4 light;
#if defined(SPECULAR) || defined(OFFSETMAPPING) || defined(REFLECTCUBEMASK) || defined(PBR)
varying vec3 eyevector;
#endif
#if defined(PBR)||defined(REFLECTCUBEMASK)
	varying mat3 invsurface;
#endif
#ifdef TESS
varying vec3 vertex;
varying vec3 normal;
#endif

void main ()
{
	light.rgba = vec4(e_light_ambient, 1.0);

#ifdef NONORMALS
	vec3 n, w;
	gl_Position = skeletaltransform_w(w);
	n = vec3(0.0);
#else
	vec3 n, s, t, w;
	gl_Position = skeletaltransform_wnst(w,n,s,t);
	n = normalize(n);
	s = normalize(s);
	t = normalize(t);
	#ifndef PBR
		#ifdef EIGHTBIT
			//doesn't darken in the shade, only gets brighter in the light (overbrighting)
			light.rgb += max(0.0,dot(n,e_light_dir)) * e_light_mul;
		#else
			//_DOES_ get darker in the shade, despite the light not lighting it at all....
			float d = dot(n,e_light_dir);
			if (d < 0.0)
				d *= 13.0/44.0;	//a wtfery factor to approximate glquake's anorm_dots.h
			light.rgb += d * e_light_mul;
		#endif
	#else
		light.rgb = vec3(1.0);
	#endif
#endif

#if defined(SPECULAR)||defined(OFFSETMAPPING) || defined(REFLECTCUBEMASK) || defined(PBR)
	vec3 eyeminusvertex = e_eyepos - w.xyz;
	eyevector.x = dot(eyeminusvertex, s.xyz);
	eyevector.y = dot(eyeminusvertex, t.xyz);
	eyevector.z = dot(eyeminusvertex, n.xyz);
#endif
#if defined(PBR) || defined(REFLECTCUBEMASK)
	invsurface = mat3(s, t, n);
#endif

	tc = v_texcoord;

#ifdef VC
	light *= v_colour;
#endif

//FIXME: Software rendering imitation should possibly push out normals by half a pixel or something to approximate software's over-estimation of distant model sizes (small models are drawn using JUST their verticies using the nearest pixel, which results in larger meshes)

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
affine in vec2 tc[];
affine out vec2 t_tc[];
in vec4 light[];
out vec4 t_light[];
#if defined(SPECULAR) || defined(OFFSETMAPPING) || defined(REFLECTCUBEMASK) || defined(PBR)
in vec3 eyevector[];
out vec3 t_eyevector[];
#endif
#ifdef REFLECTCUBEMASK
in mat3 invsurface[];
out mat3 t_invsurface[];
#endif
void main()
{
	//the control shader needs to pass stuff through
#define id gl_InvocationID
	t_vertex[id] = vertex[id];
	t_normal[id] = normal[id];
	t_tc[id] = tc[id];
	t_light[id] = light[id];
#if defined(SPECULAR) || defined(OFFSETMAPPING) || defined(REFLECTCUBEMASK) || defined(PBR)
	t_eyevector[id] = eyevector[id];
#endif
#ifdef REFLECTCUBEMASK
	t_invsurface[id][0] = invsurface[id][0];
	t_invsurface[id][1] = invsurface[id][1];
	t_invsurface[id][2] = invsurface[id][2];
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
affine in vec2 t_tc[];
affine out vec2 tc;
in vec4 t_light[];
out vec4 light;
#if defined(SPECULAR) || defined(OFFSETMAPPING) || defined(REFLECTCUBEMASK) || defined(PBR)
in vec3 t_eyevector[];
out vec3 eyevector;
#endif
#ifdef REFLECTCUBEMASK
in mat3 t_invsurface[];
out mat3 invsurface;
#endif

#define LERP(a) (gl_TessCoord.x*a[0] + gl_TessCoord.y*a[1] + gl_TessCoord.z*a[2])
void main()
{
#define factor 1.0
	tc = LERP(t_tc);
	vec3 w = LERP(t_vertex);

	vec3 t0 = w - dot(w-t_vertex[0],t_normal[0])*t_normal[0];
	vec3 t1 = w - dot(w-t_vertex[1],t_normal[1])*t_normal[1];
	vec3 t2 = w - dot(w-t_vertex[2],t_normal[2])*t_normal[2];
	w = w*(1.0-factor) + factor*(gl_TessCoord.x*t0+gl_TessCoord.y*t1+gl_TessCoord.z*t2);

	//FIXME: we should be recalcing these here, instead of just lerping them
	light = LERP(t_light);
#if defined(SPECULAR) || defined(OFFSETMAPPING) || defined(REFLECTCUBEMASK) || defined(PBR)
	eyevector = LERP(t_eyevector);
#endif
#ifdef REFLECTCUBEMASK
	invsurface[0] = LERP(t_invsurface[0]);
	invsurface[1] = LERP(t_invsurface[1]);
	invsurface[2] = LERP(t_invsurface[2]);
#endif

	gl_Position = m_modelviewprojection * vec4(w,1.0);
}
#endif










#ifdef FRAGMENT_SHADER

#include "sys/fog.h"

#if defined(SPECULAR)
uniform float cvar_gl_specular;
#endif

#ifdef OFFSETMAPPING
#include "sys/offsetmapping.h"
#endif

#ifdef EIGHTBIT
#define s_colourmap s_t0
#endif

affine varying vec2 tc;
varying vec4 light;
#if defined(SPECULAR) || defined(OFFSETMAPPING) || defined(REFLECTCUBEMASK) || defined(PBR)
varying vec3 eyevector;
#endif
#if defined(PBR) || defined(REFLECTCUBEMASK)
	varying mat3 invsurface;
#endif

#ifdef PBR
#include "sys/pbr.h"
#if 0
vec3 getIBLContribution(PBRInfo pbrInputs, vec3 n, vec3 reflection)
{
    float mipCount = 9.0; // resolution of 512x512
    float lod = (pbrInputs.perceptualRoughness * mipCount);
    // retrieve a scale and bias to F0. See [1], Figure 3
    vec3 brdf = texture2D(u_brdfLUT, vec2(pbrInputs.NdotV, 1.0 - pbrInputs.perceptualRoughness)).rgb;
    vec3 diffuseLight = textureCube(u_DiffuseEnvSampler, n).rgb;

#ifdef USE_TEX_LOD
    vec3 specularLight = textureCubeLodEXT(u_SpecularEnvSampler, reflection, lod).rgb;
#else
    vec3 specularLight = textureCube(u_SpecularEnvSampler, reflection).rgb;
#endif

    vec3 diffuse = diffuseLight * pbrInputs.diffuseColor;
    vec3 specular = specularLight * (pbrInputs.specularColor * brdf.x + brdf.y);

    // For presentation, this allows us to disable IBL terms
    diffuse *= u_ScaleIBLAmbient.x;
    specular *= u_ScaleIBLAmbient.y;

    return diffuse + specular;
}
#endif
#endif


void main ()
{
	vec4 col, sp;

#ifdef OFFSETMAPPING
	vec2 tcoffsetmap = offsetmap(s_normalmap, tc, eyevector);
#define tc tcoffsetmap
#endif

#ifdef EIGHTBIT
	vec3 lightlev = light.rgb;
	//FIXME: with this extra flag, half the permutations are redundant.
	lightlev *= 0.5;	//counter the fact that the colourmap contains overbright values and logically ranges from 0 to 2 intead of to 1.
	float pal = texture2D(s_paletted, tc).r;	//the palette index. hopefully not interpolated.
//	lightlev -= 1.0 / 128.0;	//software rendering appears to round down, so make sure we favour the lower values instead of rounding to the nearest
	col.r = texture2D(s_colourmap, vec2(pal, 1.0-lightlev.r)).r;	//do 3 lookups. this is to cope with lit files, would be a waste to not support those.
	col.g = texture2D(s_colourmap, vec2(pal, 1.0-lightlev.g)).g;	//its not very softwarey, but re-palettizing is ugly.
	col.b = texture2D(s_colourmap, vec2(pal, 1.0-lightlev.b)).b;	//without lits, it should be identical.
	col.a = (pal<1.0)?light.a:0.0;
#else
	col = texture2D(s_diffuse, tc);
	#ifdef UPPER
		vec4 uc = texture2D(s_upper, tc);
		col.rgb += uc.rgb*e_uppercolour*uc.a;
	#endif
	#ifdef LOWER
		vec4 lc = texture2D(s_lower, tc);
		col.rgb += lc.rgb*e_lowercolour*lc.a;
	#endif

	col *= factor_base;

    #ifndef IOR
        #define IOR 1.5 //Index Of Reflection.
    #endif
    #define dielectricSpecular pow(((IOR - 1.0)/(IOR + 1.0)),2.0)
	#ifdef SPECULAR
		vec4 specs = texture2D(s_specular, tc)*factor_spec;
		#ifdef ORM
			#define occlusion specs.r
			#define roughness clamp(specs.g, 0.04, 1.0)
			#define metalness specs.b
			#define gloss 1.0 //sqrt(1.0-roughness)
			#define ambientrgb (specrgb+col.rgb)
			vec3 specrgb = mix(vec3(dielectricSpecular), col.rgb, metalness);
			col.rgb = col.rgb * (1.0 - dielectricSpecular) * (1.0-metalness);
		#elif defined(SG) //pbr-style specular+glossiness, without occlusion
			//occlusion needs to be baked in. :(
			#define roughness (1.0-specs.a)
			#define gloss (specs.a)
			#define specrgb specs.rgb
			#define ambientrgb (specrgb+col.rgb)
		#else	//blinn-phong
			#define roughness (1.0-specs.a)
			#define gloss specs.a
			#define specrgb specs.rgb
			#define ambientrgb col.rgb
		#endif
	#else
		#define roughness 0.3
		#define specrgb vec3(1.0) //vec3(dielectricSpecular)
		#define ambientrgb col.rgb
	#endif

	#ifdef BUMP
		#ifdef PBR	//to modelspace
			vec3 bumps = normalize(invsurface * (texture2D(s_normalmap, tc).rgb*2.0 - 1.0));
		#else	//stay in tangentspace
			vec3 bumps = normalize(vec3(texture2D(s_normalmap, tc)) - 0.5);
		#endif
	#else
		#ifdef PBR	//to modelspace
			#define bumps normalize(invsurface[2])
		#else	//tangent space
			#define bumps vec3(0.0, 0.0, 1.0)
		#endif
	#endif

	#ifdef PBR
		//move everything to model space
		col.rgb = DoPBR(bumps, normalize(eyevector), -e_light_dir, roughness, col.rgb, specrgb, vec3(0.0,1.0,1.0))*e_light_mul + e_light_ambient*.25*ambientrgb;
	#elif defined(gloss)
		vec3 halfdir = normalize(normalize(eyevector) - e_light_dir);
		float specmag = pow(max(dot(halfdir, bumps), 0.0), FTE_SPECULAR_EXPONENT * gloss);
		col.rgb += FTE_SPECULAR_MULTIPLIER * specmag * specrgb;
	#endif

	#ifdef REFLECTCUBEMASK
		vec3 rtc = reflect(-eyevector, bumps);
		#ifndef PBR
			rtc = rtc.x*invsurface[0] + rtc.y*invsurface[1] + rtc.z*invsurface[2];
		#endif
		rtc = (m_model * vec4(rtc.xyz,0.0)).xyz;
		col.rgb += texture2D(s_reflectmask, tc).rgb * textureCube(s_reflectcube, rtc).rgb;
	#endif

#ifdef OCCLUDE
	col.rgb *= texture2D(s_occlusion, tc).r;	
#elif defined(occlusion) && !defined(NOOCCLUDE)
	col.rgb *= occlusion;
#endif
	col *= light * e_colourident;

	#ifdef FULLBRIGHT
		vec4 fb = texture2D(s_fullbright, tc);
//		col.rgb = mix(col.rgb, fb.rgb, fb.a);
		col.rgb += fb.rgb * fb.a * e_glowmod.rgb * factor_emit.rgb;
	#elif defined(PBR)
		col.rgb += e_glowmod.rgb * factor_emit.rgb;
	#endif
#endif

#ifdef ALPHATEST
    if (!(col.a ALPHATEST))
        discard;
#elif defined(MASK)
	#if defined(MASKLT)
		if (col.a < MASK)
			discard;
	#else
		if (col.a >= MASK)
			discard;
	#endif
    col.a = 1.0;    //alpha blending AND alpha testing usually looks stupid, plus it screws up our fog.
#endif

	gl_FragColor = fog4(col);
}
#endif

