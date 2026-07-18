
$ifndef instance
	OPTION ARB_position_invariant;
$endif

TEMP	bitangent;
TEMP	R0, R1, R2;

$ifdef instance
	TEMP _pos;
	TEMP _nrm;
	TEMP _tan;
	DP4 _pos.x, vertex.texcoord[5], $positionAttrib;
	DP4 _pos.y, vertex.texcoord[6], $positionAttrib;
	DP4 _pos.z, vertex.texcoord[7], $positionAttrib;
	MOV _pos.w, $positionAttrib.w;

	DP3 _nrm.x, vertex.texcoord[5], $normalAttrib;
	DP3 _nrm.y, vertex.texcoord[6], $normalAttrib;
	DP3 _nrm.z, vertex.texcoord[7], $normalAttrib;
	
	MOV result.texcoord[6].w, vertex.texcoord[2];
	
	PARAM  mvp[4]       = { state.matrix.mvp };
	OUTPUT oPos         = result.position;
	# Transform the vertex to clip coordinates. 
	DP4   oPos.x, mvp[0], _pos;
	DP4   oPos.y, mvp[1], _pos;
	DP4   oPos.z, mvp[2], _pos;
	DP4   oPos.w, mvp[3], _pos;
$else
	ATTRIB _pos = $positionAttrib;
	$if !r_32ByteVtx
		ATTRIB _nrm = $normalAttrib;
	$else
		TEMP _nrm;
		MOV _nrm, $normalAttrib;
	$endif
$endif

PARAM	defaultTexCoord = { 0, 0.5, 0, 1 };

# calculate vector to light in R0
SUB		R0, $lightOrigin, _pos;
DP3		R0.w, R0, R0;
RSQ		R0.w, R0.w;
MUL		R0.xyz, R0, R0.w;

DP3		result.texcoord[0].z, _nrm, R0;

# texture 2 has one texgen
MOV		result.texcoord[2], defaultTexCoord;
DP4		result.texcoord[2].x, _pos, $lightFalloff_s;

# texture 3 has three texgens
DP4		result.texcoord[3].x, _pos, $lightProject_s;
DP4		result.texcoord[3].y, _pos, $lightProject_t;
DP4		result.texcoord[3].w, _pos, $lightProject_q;

# textures 4 takes the base coordinates by the texture matrix
MOV		result.texcoord[4], defaultTexCoord;
DP4		result.texcoord[4].x, $texCoordAttrib, $diffuseMatrix_s;
DP4		result.texcoord[4].y, $texCoordAttrib, $diffuseMatrix_t;

# generate the vertex color, which can be 1.0, color, or 1.0 - color
# for 1.0 : env[16] = 0, env[17] = 1
# for color : env[16] = 1, env[17] = 0
# for 1.0-color : env[16] = -1, env[17] = 1
MAD		result.color, $colorAttrib, $colorModulate, $colorAdd;

$if ( r_megaDrawMethod != 0 )
TEMP _F1, _F2;
SUB		_F2, _pos, $viewOrigin;
DP3		_F1.x, _F2, _F2;
RSQ		_F1.y, _F1.x;
MUL		_F1.x, _F1.y, _F1.x;
MAD		result.color.secondary, _F1.x, $fogDepths.z, $fogDepths.w; 
$endif
