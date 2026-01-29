!!permu FRAMEBLEND
!!permu UPPERLOWER
//!!permu FULLBRIGHT
!!samps diffuse upper lower
// fullbright

struct a2v
{
	float3 pos: POSITION0;
#ifdef FRAMEBLEND
	float3 pos2: POSITION1;
#endif
	float2 tc: TEXCOORD0;
	float3 normal: NORMAL;
};
struct v2f
{
#ifndef FRAGMENT_SHADER
	float4 pos: POSITION;
#endif
	float2 tc: TEXCOORD0;
	float3 light: TEXCOORD1;
};

//#include <ftedefs.h>

#ifdef VERTEX_SHADER
float3 e_light_dir;
float3 e_light_mul;
float3 e_light_ambient;
float2 e_vblend;
float4x4  m_model;
float4x4  m_view;
float4x4  m_projection;
	v2f main (a2v inp)
	{
		v2f outp;
		float4 pos;
#ifdef FRAMEBLEND
		pos = float4(e_vblend.x*inp.pos + e_vblend.y*inp.pos2, 1);
#else
		pos = float4(inp.pos, 1);
#endif
		outp.pos = mul(m_model, pos);
		outp.pos = mul(m_view, outp.pos);
		outp.pos = mul(m_projection, outp.pos);

		float d = dot(inp.normal, e_light_dir);
		outp.light = e_light_ambient + (d * e_light_mul);
		outp.tc = inp.tc.xy;
		return outp;
	}
#endif
#ifdef FRAGMENT_SHADER
float4 e_colourident;
float3 e_uppercolour;
float3 e_lowercolour;
sampler s_diffuse; /*diffuse*/
sampler s_upper; /*upper*/
sampler s_lower; /*lower*/
sampler s_fullbright; /*fullbright*/
	float4 main (v2f inp) : SV_TARGET
	{
		float4 col;
		col = tex2D(s_diffuse, inp.tc);
#ifdef UPPER
		float4 uc = tex2D(s_upper, inp.tc);
		col.rgb += uc.rgb*e_uppercolour * uc.a;
#endif
#ifdef LOWER
		float4 lc = tex2D(s_lower, inp.tc);
		col.rgb += lc.rgb*e_lowercolour * lc.a;
#endif
		col.rgb *= inp.light;
#ifdef FULLBRIGHT
		float4 fb = tex2D(s_fullbright, inp.tc);
		col.rgb = lerp(col.rgb, fb.rgb, fb.a);
#endif
		return col * e_colourident;
//		return fog4(col * e_colourident);
	}
#endif
