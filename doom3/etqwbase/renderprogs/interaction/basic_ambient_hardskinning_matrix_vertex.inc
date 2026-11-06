TEMP	position, normal, tangent, bitangent;
TEMP	R0, R1, R2;

PARAM	defaultTexCoord = { 0, 0.5, 0, 1 };

useTemplate hardSkinningMatrix_ARB< "position", "normal", "tangent", "bitangent" >

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
DP3		result.texcoord[5].x, $transposedModelMatrix_x, tangent;
DP3		result.texcoord[6].x, $transposedModelMatrix_y, tangent;
DP3		result.texcoord[7].x, $transposedModelMatrix_z, tangent;

DP3		result.texcoord[5].y, $transposedModelMatrix_x, bitangent;
DP3		result.texcoord[6].y, $transposedModelMatrix_y, bitangent;
DP3		result.texcoord[7].y, $transposedModelMatrix_z, bitangent;

DP3		result.texcoord[5].z, $transposedModelMatrix_x, normal;
DP3		result.texcoord[6].z, $transposedModelMatrix_y, normal;
DP3		result.texcoord[7].z, $transposedModelMatrix_z, normal;

# calculate vector to eye
DP4		R0.x, $transposedModelMatrix_x, position;
DP4		R0.y, $transposedModelMatrix_y, position;
DP4		R0.z, $transposedModelMatrix_z, position;

# normalize
#TEMP	viewtovec;
SUB		R1.xyz, R0, $viewOriginWorld;
DP3		R1.w, R1, R1;
RSQ		R0.w, R1.w;
MUL		result.texcoord[2], R1, -R0.w;

$if rimlightvtx
TEMP	_V0, _V1;
SUB		_V0, $viewOrigin, position;
DP3		_V1.x, _V0, _V0;
RSQ		_V1.x, _V1.x;
MUL		_V0.xyz, _V0, _V1.x;

DP3		_V1.x, _V0, normal;
MAX		_V1.x, _V1.x, 0;

SUB		_V1.x, 1, _V1.x;
MUL		_V1.x, _V1.x, _V1.x;
MUL		_V1.x, _V1.x, _V1.x;
MUL		result.color.secondary, _V1.x, 2;
$endif

#ifdef enable_detail_fade
PARAM  mv[4]       = { state.matrix.modelview };
DP4   R0.w, -mv[2], position;
SUB   R0.w, R0.w, $detailFade.x;
MAD result.texcoord[1].w, R0.w, $detailFade.y, 1;
#endif

$if ( r_megaDrawMethod != 0 )
SUB		R2, position, $viewOrigin;
DP3		R1.x, R2, R2;
RSQ		R1.y, R1.x;
MUL		R1.x, R1.y, R1.x;
MAD		result.color.secondary, R1.x, $fogDepths.z, $fogDepths.w; 
$endif
