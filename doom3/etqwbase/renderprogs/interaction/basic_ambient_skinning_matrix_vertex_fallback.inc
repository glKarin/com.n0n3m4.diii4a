TEMP	bitangent;
TEMP	R0, R1, R2, position, _nrm;

useTemplate skinningMatrix_ARB< "position", "_nrm" >

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
SUB		R2, position, $viewOrigin;
DP3		R1.x, R2, R2;
RSQ		R1.y, R1.x;
MUL		R1.x, R1.y, R1.x;
MAD		result.color.secondary, R1.x, $fogDepths.z, $fogDepths.w; 
$endif
