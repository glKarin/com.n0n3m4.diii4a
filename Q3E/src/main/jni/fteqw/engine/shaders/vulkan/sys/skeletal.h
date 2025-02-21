#ifdef SKELETAL
	vec4 skeletaltransform()
	{
		mat3x4 wmat;
		wmat = m_bones[int(v_bone.x)] * v_weight.x;
		wmat += m_bones[int(v_bone.y)] * v_weight.y;
		wmat += m_bones[int(v_bone.z)] * v_weight.z;
		wmat += m_bones[int(v_bone.w)] * v_weight.w;
		return m_modelviewprojection * vec4(vec4(v_position.xyz, 1.0) * wmat, 1.0);
	}
	vec4 skeletaltransform_nst(out vec3 n, out vec3 t, out vec3 b)
	{
		mat3x4 wmat;
		wmat = m_bones[int(v_bone.x)] * v_weight.x;
		wmat += m_bones[int(v_bone.y)] * v_weight.y;
		wmat += m_bones[int(v_bone.z)] * v_weight.z;
		wmat += m_bones[int(v_bone.w)] * v_weight.w;
		n = vec4(v_normal.xyz, 0.0) * wmat;
		t = vec4(v_svector.xyz, 0.0) * wmat;
		b = vec4(v_tvector.xyz, 0.0) * wmat;
		return m_modelviewprojection * vec4(vec4(v_position.xyz, 1.0) * wmat, 1.0);
	}
	vec4 skeletaltransform_wnst(out vec3 w, out vec3 n, out vec3 t, out vec3 b)
	{
		mat3x4 wmat;
		wmat = m_bones[int(v_bone.x)] * v_weight.x;
		wmat += m_bones[int(v_bone.y)] * v_weight.y;
		wmat += m_bones[int(v_bone.z)] * v_weight.z;
		wmat += m_bones[int(v_bone.w)] * v_weight.w;
		n = vec4(v_normal.xyz, 0.0) * wmat;
		t = vec4(v_svector.xyz, 0.0) * wmat;
		b = vec4(v_tvector.xyz, 0.0) * wmat;
		w = vec4(v_position.xyz, 1.0) * wmat;
		return m_modelviewprojection * vec4(w, 1.0);
	}
	vec4 skeletaltransform_n(out vec3 n)
	{
		mat3x4 wmat;
		wmat = m_bones[int(v_bone.x)] * v_weight.x;
		wmat += m_bones[int(v_bone.y)] * v_weight.y;
		wmat += m_bones[int(v_bone.z)] * v_weight.z;
		wmat += m_bones[int(v_bone.w)] * v_weight.w;
		n = vec4(v_normal.xyz, 0.0) * wmat;
		return m_modelviewprojection * vec4(vec4(v_position.xyz, 1.0) * wmat, 1.0);
	}
#else
	#define skeletaltransform ftetransform
	vec4 skeletaltransform_wnst(out vec3 w, out vec3 n, out vec3 t, out vec3 b)
	{
		n = v_normal;
		t = v_svector;
		b = v_tvector;
		w = v_position.xyz;
		return ftetransform();
	}
	vec4 skeletaltransform_nst(out vec3 n, out vec3 t, out vec3 b)
	{
		n = v_normal;
		t = v_svector;
		b = v_tvector;
		return ftetransform();
	}
	vec4 skeletaltransform_n(out vec3 n)
	{
		n = v_normal;
		return ftetransform();
	}
#endif