!!permu UPPERLOWER
!!samps diffuse upper lower fullbright

struct a2v
{
	float4 pos: POSITION;
	float2 tc: TEXCOORD0;
	float3 normal: NORMAL;
};
struct v2f
{
	float4 pos: SV_POSITION;
	float2 tc: TEXCOORD0;
	float3 light: TEXCOORD1;
};

#include <ftedefs.h>

#ifdef VERTEX_SHADER
//attribute vec2 v_texcoord;
//uniform vec3 e_light_dir;
//uniform vec3 e_light_mul;
//uniform vec3 e_light_ambient;
	v2f main (a2v inp)
	{
		v2f outp;
		outp.pos = mul(m_model, inp.pos);
		outp.pos = mul(m_view, outp.pos);
		outp.pos = mul(m_projection, outp.pos);
		outp.light = e_light_ambient + (dot(inp.normal,e_light_dir)*e_light_mul);
		outp.tc = inp.tc.xy;
		return outp;
	}
#endif
#ifdef FRAGMENT_SHADER
	Texture2D t_diffuse		: register(t0);
#ifdef UPPER
	Texture2D t_upper		: register(t1);
	Texture2D t_lower		: register(t2);
	Texture2D t_fullbright	: register(t3);
#else
	Texture2D t_fullbright	: register(t1);
#endif

	SamplerState SampleType;

	float4 main (v2f inp) : SV_TARGET
	{
		float4 col;
		col = t_diffuse.Sample(SampleType, inp.tc);

		#ifdef MASK
			#ifndef MASKOP
				#define MASKOP >=	//drawn if (alpha OP ref) is true.
			#endif
			//support for alpha masking
			if (!(col.a MASKOP MASK))
				discard;
		#endif

#ifdef UPPER
		float4 uc = t_upper.Sample(SampleType, inp.tc);
		col.rgb += uc.rgb*e_uppercolour.rgb*uc.a;
#endif
#ifdef LOWER
		float4 lc = t_lower.Sample(SampleType, inp.tc);
		col.rgb += lc.rgb*e_lowercolour.rgb*lc.a;
#endif
		col.rgb *= inp.light;
//#ifdef FULLBRIGHT
		float4 fb = t_fullbright.Sample(SampleType, inp.tc)*e_glowmod;
//		col.rgb = lerp(col.rgb, fb.rgb, fb.a);	//matches vanilla quake...
		col.rgb += fb.rgb * fb.a;				//but nothing expects it to.
//#endif
		col *= e_colourmod;
//		col = fog4(col);
		return col;
	}
#endif
