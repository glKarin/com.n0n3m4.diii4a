TEMP	bitangent;
TEMP	R0, R1, R2, position, _nrm;

useTemplate hardSkinningMatrix_ARB< "position", "_nrm" >

PARAM	defaultTexCoord = { 0, 0.5, 0, 1 };

# calculate vector to light in R0
SUB		R0, $lightOrigin, position;
DP3		R0.w, R0, R0;
RSQ		R0.w, R0.w;
MUL		R0.xyz, R0, R0.w;

DP3		result.texcoord[0].z, _nrm, R0;

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

# generate the vertex color, which can be 1.0, color, or 1.0 - color
# for 1.0 : env[16] = 0, env[17] = 1
# for color : env[16] = 1, env[17] = 0
# for 1.0-color : env[16] = -1, env[17] = 1
MAD		result.color, $colorAttrib, $colorModulate, $colorAdd;


$if ( r_megaDrawMethod != 0 )
TEMP _F1, _F2;
SUB		_F2, position, $viewOrigin;
DP3		_F1.x, _F2, _F2;
RSQ		_F1.y, _F1.x;
MUL		_F1.x, _F1.y, _F1.x;
MAD		result.color.secondary, _F1.x, $fogDepths.z, $fogDepths.w; 
$endif