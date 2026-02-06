!!permu BUMP
!!permu SPECULAR
!!permu OFFSETMAPPING
!!permu SKELETAL
!!permu FOG
!!samps diffuse

//	texture units:
//	s0=diffuse, s1=normal, s2=specular, s3=shadowmap
//	custom modifiers:
//	PCF(shadowmap)
//	CUBE(projected cubemap)



	struct a2v
	{
		float4 pos: POSITION;
		float3 tc: TEXCOORD0;
		float3 n: NORMAL0;
		float3 s: TANGENT0;
		float3 t: BINORMAL0;
	};
	struct v2f
	{
		#ifndef FRAGMENT_SHADER
		float4 pos: POSITION;
		#endif
		float3 tc: TEXCOORD0;
		float3 lpos: TEXCOORD1;
	};

#ifdef VERTEX_SHADER
	float4x4  m_modelviewprojection;
	float3 l_lightposition;
	v2f main (a2v inp)
	{
		v2f outp;
		outp.pos = mul(m_modelviewprojection, inp.pos);
		outp.tc = inp.tc;

		float3 lightminusvertex = l_lightposition - inp.pos.xyz;
		outp.lpos.x = dot(lightminusvertex, inp.s.xyz);
		outp.lpos.y = dot(lightminusvertex, inp.t.xyz);
		outp.lpos.z = dot(lightminusvertex, inp.n.xyz);
		return outp;
	}
#endif

#ifdef FRAGMENT_SHADER
	sampler s_diffuse;
	float l_lightradius;
	float3 l_lightcolour;
	float4 main (v2f inp) : COLOR0
	{
		float3 col = l_lightcolour;
		col *= max(1.0 - dot(inp.lpos, inp.lpos)/(l_lightradius*l_lightradius), 0.0);
#ifdef FLAT
		float3 diff = FLAT;
#else
		float3 diff = tex2D(s_diffuse, inp.tc);
#endif
		return float4(diff * col, 1);
	}
#endif
