
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

# textures 4 takes the base coordinates by the texture matrix
MOV		result.texcoord[4], defaultTexCoord;
DP4		result.texcoord[4].x, $texCoordAttrib, $diffuseMatrix_s;
DP4		result.texcoord[4].y, $texCoordAttrib, $diffuseMatrix_t;

# generate the vertex color, which can be 1.0, color, or 1.0 - color
MAD		result.color, $colorAttrib, $colorModulate, $colorAdd;

DP3		result.texcoord[5].x, $transposedModelMatrix_x, _nrm;
DP3		result.texcoord[5].y, $transposedModelMatrix_y, _nrm;
DP3		result.texcoord[5].z, $transposedModelMatrix_z, _nrm;

$if ( r_megaDrawMethod != 0 )
SUB		R2, _pos, $viewOrigin;
DP3		R1.x, R2, R2;
RSQ		R1.y, R1.x;
MUL		R1.x, R1.y, R1.x;
MAD		result.color.secondary, R1.x, $fogDepths.z, $fogDepths.w; 
$endif
