
$ifndef instance
	$ifndef deform
		OPTION ARB_position_invariant;
	$endif
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
	
	DP3 _tan.x, vertex.texcoord[5], $tangentAttrib;
	DP3 _tan.y, vertex.texcoord[6], $tangentAttrib;
	DP3 _tan.z, vertex.texcoord[7], $tangentAttrib;

	MOV result.texcoord[6].w, vertex.texcoord[2];

	PARAM  mvp[4]       = { state.matrix.mvp };
	OUTPUT oPos         = result.position;
	# Transform the vertex to clip coordinates. 
	DP4   oPos.x, mvp[0], _pos;
	DP4   oPos.y, mvp[1], _pos;
	DP4   oPos.z, mvp[2], _pos;
	DP4   oPos.w, mvp[3], _pos;
$else
	$ifdef deform
		TEMP _pos;
		PARAM  mvp[4]       = { state.matrix.mvp };
		OUTPUT oPos         = result.position;
		# Transform the vertex to clip coordinates. 
		MUL		R0.x, $parameters.x, $colorAttrib.w;
		MAD   _pos.xyz, R0.x, $normalAttrib, $positionAttrib;
		MOV	  _pos.w, $positionAttrib.w;
		DP4   oPos.x, mvp[0], _pos;
		DP4   oPos.y, mvp[1], _pos;
		DP4   oPos.z, mvp[2], _pos;
		DP4   oPos.w, mvp[3], _pos;
	$else
		ATTRIB _pos = $positionAttrib;
	$endif
	$if !r_32ByteVtx
	ATTRIB _nrm = $normalAttrib;
	ATTRIB _tan = $tangentAttrib;
	$else
	TEMP _nrm, _tan;
	MOV _nrm, $normalAttrib;
	MOV _tan, $tangentAttrib;
	$endif
$endif
        
PARAM	defaultTexCoord = { 0, 0.5, 0, 1 };

XPD		bitangent, _nrm, _tan;
MUL		bitangent, bitangent, $tangentAttrib.w;

# textures 1 takes the base coordinates by the texture matrix
MOV		result.texcoord[1], defaultTexCoord;
DP4		result.texcoord[1].x, $texCoordAttrib, $bumpMatrix_s;
DP4		result.texcoord[1].y, $texCoordAttrib, $bumpMatrix_t;

# textures 4 takes the base coordinates by the texture matrix
MOV		result.texcoord[4], defaultTexCoord;
DP4		result.texcoord[4].x, $texCoordAttrib, $diffuseMatrix_s;
DP4		result.texcoord[4].y, $texCoordAttrib, $diffuseMatrix_t;

# generate the vertex color, which can be 1.0, color, or 1.0 - color
MAD		result.color, $colorAttrib, $colorModulate, $colorAdd;

# build tangent space -> world space conversion matrix
DP3		result.texcoord[5].x, $transposedModelMatrix_x, _tan;
DP3		result.texcoord[6].x, $transposedModelMatrix_y, _tan;
DP3		result.texcoord[7].x, $transposedModelMatrix_z, _tan;

DP3		result.texcoord[5].y, $transposedModelMatrix_x, bitangent;
DP3		result.texcoord[6].y, $transposedModelMatrix_y, bitangent;
DP3		result.texcoord[7].y, $transposedModelMatrix_z, bitangent;

DP3		result.texcoord[5].z, $transposedModelMatrix_x, _nrm;
DP3		result.texcoord[6].z, $transposedModelMatrix_y, _nrm;
DP3		result.texcoord[7].z, $transposedModelMatrix_z, _nrm;

# calculate vector to eye
DP4		R0.x, $transposedModelMatrix_x, _pos;
DP4		R0.y, $transposedModelMatrix_y, _pos;
DP4		R0.z, $transposedModelMatrix_z, _pos;

# normalize
TEMP	viewtovec;
SUB		viewtovec.xyz, R0, $viewOriginWorld;
DP3		viewtovec.w, viewtovec, viewtovec;
RSQ		R0.w, viewtovec.w;
MUL		result.texcoord[2], viewtovec, -R0.w;
#MOV		result.texcoord[2], R0;

$ifdef 	parallax
	# calculate vector to light in R0
	SUB		R0, $viewOrigin, _pos;

	# put into texture space for TEX6
	DP3		result.texcoord[0].x, _tan, R0;
	DP3		result.texcoord[0].y, bitangent, R0;
	DP3		result.texcoord[0].z, _nrm, R0;
$endif

#ifdef enable_detail_fade
PARAM  mv[4]       = { state.matrix.modelview };
DP4   R0.w, -mv[2], _pos;
SUB   R0.w, R0.w, $detailFade.x;
MAD result.texcoord[1].w, R0.w, $detailFade.y, 1;
#endif

$if ( r_megaDrawMethod != 0 )
SUB		R2, _pos, $viewOrigin;
DP3		R1.x, R2, R2;
RSQ		R1.y, R1.x;
MUL		R1.x, R1.y, R1.x;
MAD		result.color.secondary, R1.x, $fogDepths.z, $fogDepths.w; 
$endif
