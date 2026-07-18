TEMP	position, normal, tangent, bitangent;
TEMP	R0, R1, R2;

PARAM	defaultTexCoord = { 0, 0.5, 0, 1 };

useTemplate skinningMatrix_ARB< "position", "normal", "tangent", "bitangent" >

# calculate vector to light in R0
SUB		R0, $lightOrigin, position;
MUL		R0, R0, 0.1;

# put into texture space for TEX0
DP3		result.texcoord[0].x, tangent, R0;
DP3		result.texcoord[0].y, bitangent, R0;
DP3		result.texcoord[0].z, normal, R0;

# textures 1 takes the base coordinates by the texture matrix
MOV		result.texcoord[1], defaultTexCoord;
DP4		result.texcoord[1].x, $texCoordAttrib, $bumpMatrix_s;
DP4		result.texcoord[1].y, $texCoordAttrib, $bumpMatrix_t;

# texture 2 has one texgen
MOV		result.texcoord[2], defaultTexCoord;
DP4		result.texcoord[2].x, position, $lightFalloff_s;

# texture 3 has three texgens
DP4		result.texcoord[3].x, position, $lightProject_s;
DP4		result.texcoord[3].y, position, $lightProject_t;
DP4		result.texcoord[3].w, position, $lightProject_q;

# textures 4 takes the base coordinates by the texture matrix
MOV		result.texcoord[4], defaultTexCoord;
DP4		result.texcoord[4].x, $texCoordAttrib, $diffuseMatrix_s;
DP4		result.texcoord[4].y, $texCoordAttrib, $diffuseMatrix_t;

# textures 5 takes the base coordinates by the texture matrix
MOV		result.texcoord[5], defaultTexCoord;
DP4		result.texcoord[5].x, $texCoordAttrib, $specularMatrix_s;
DP4		result.texcoord[5].y, $texCoordAttrib, $specularMatrix_t;

$if !r_usePerFragmentSpecular
	# texture 6's texcoords will be the halfangle in texture space

	# calculate normalized vector to light in R0
	SUB		R0, $lightOrigin, position;
	DP3		R1, R0, R0;
	RSQ		R1, R1.x;
	MUL		R0, R0, R1.x;

	# calculate normalized vector to viewer in R1
	SUB		R1, $viewOrigin, position;
	DP3		R2, R1, R1;
	RSQ		R2, R2.x;
	MUL		R1, R1, R2.x;

	# add together to become the half angle vector in object space (non-normalized)
	ADD		R0, R0, R1;

	# put into texture space
	DP3		result.texcoord[6].x, tangent, R0;
	DP3		result.texcoord[6].y, bitangent, R0;
	DP3		result.texcoord[6].z, normal, R0;
$else
	# calculate vector to light in R0
	SUB		R0, $viewOrigin, $positionAttrib;

	# put into texture space for TEX6
	DP3		result.texcoord[6].x, tangent, R0;
	DP3		result.texcoord[6].y, bitangent, R0;
	DP3		result.texcoord[6].z, normal, R0;
$endif

# generate the vertex color, which can be 1.0, color, or 1.0 - color
# for 1.0 : env[16] = 0, env[17] = 1
# for color : env[16] = 1, env[17] = 0
# for 1.0-color : env[16] = -1, env[17] = 1
MAD		result.color, $colorAttrib, $colorModulate, $colorAdd;

$ifdef enable_detail_fade
PARAM  mv[4]       = { state.matrix.modelview };
DP4   R0.w, -mv[2], position;
SUB   R0.w, R0.w, $detailFade.x;
MAD result.texcoord[1].w, R0.w, $detailFade.y, 1;
$endif

$if ( r_megaDrawMethod != 0 )
SUB		R2, position, $viewOrigin;
DP3		R1.x, R2, R2;
RSQ		R1.y, R1.x;
MUL		R1.x, R1.y, R1.x;
MAD		result.color.secondary, R1.x, $fogDepths.z, $fogDepths.w; 
$endif

#ifdef enable_separate_depth
DP4		result.texcoord[2].z, $texCoordAttrib, $maskMatrix_s;
DP4		result.texcoord[2].w, $texCoordAttrib, $maskMatrix_t;
#endif