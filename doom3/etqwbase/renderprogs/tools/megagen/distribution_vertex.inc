OPTION ARB_position_invariant;

PARAM	identityModelMatrix_0 = { 1, 0, 0, 0 };
PARAM	identityModelMatrix_1 = { 0, -1, 0, 0 };	# texture coordinates have a flipped y axis compared to world space
PARAM	identityModelMatrix_2 = { 0, 0, 1, 0 };
#PARAM	identityModelMatrix_3 = { 0, 0, 0, 1 };

TEMP colorTexCoordFinal;
TEMP tempTexCoord;
TEMP tempTexCoord2;
TEMP tangentSpaceMatrix_0, tangentSpaceMatrix_1, tangentSpaceMatrix_2;
TEMP R0, R1;

# Build tangent space matrix (rotated from identity model matrix) and transpose it

MOV tangentSpaceMatrix_0, { 1, 0, 0, 0 };		# tangents[ 0 ]
MOV tangentSpaceMatrix_1, { 0, 1, 0, 0 };		# tangents[ 1 ]
MOV tangentSpaceMatrix_2, { 0, 0, 1, 0 };		# normal

MOV R1, tangentSpaceMatrix_0;
MUL R0.x, R1.x, program.local[1].y;
MUL R0.y, R1.y, program.local[1].x;
SUB tangentSpaceMatrix_0.x, R0.x, R0.y;						# tangentSpaceMatrix_0.x, transpose to result.texcoord[5].x
MUL R0.x, R1.y, program.local[1].y;
MUL R0.y, R1.x, program.local[1].x;
SUB tangentSpaceMatrix_0.y, R0.x, R0.y;						# tangentSpaceMatrix_0.y, transpose to result.texcoord[6].x
MOV tangentSpaceMatrix_0.z, R1.z;							# tangentSpaceMatrix_0.z, transpose to result.texcoord[7].x

MOV R1, tangentSpaceMatrix_1;
MUL R0.x, R1.x, program.local[1].y;
MUL R0.y, R1.y, program.local[1].x;
SUB tangentSpaceMatrix_1.x, R0.x, R0.y;						# tangentSpaceMatrix_1.x, transpose to result.texcoord[5].y
MUL R0.x, R1.y, program.local[1].y;
MUL R0.y, R1.x, program.local[1].x;
SUB tangentSpaceMatrix_1.y, R0.x, R0.y;						# tangentSpaceMatrix_1.y, transpose to result.texcoord[6].y
MOV tangentSpaceMatrix_1.z, R1.z;							# tangentSpaceMatrix_1.z, transpose to result.texcoord[7].y

# transpose
MOV result.texcoord[5].x, tangentSpaceMatrix_0.x;
MOV result.texcoord[6].x, tangentSpaceMatrix_0.y;
MOV result.texcoord[7].x, tangentSpaceMatrix_0.z;

MUL result.texcoord[5].y, tangentSpaceMatrix_1.x, -1;
MOV result.texcoord[6].y, tangentSpaceMatrix_1.y;
MOV result.texcoord[7].y, tangentSpaceMatrix_1.z;

MOV result.texcoord[5].z, tangentSpaceMatrix_2.x;
MOV result.texcoord[6].z, tangentSpaceMatrix_2.y;
MOV result.texcoord[7].z, tangentSpaceMatrix_2.z;

#######################################################

# rotate the texture
SUB tempTexCoord, vertex.texcoord[1], 0.5;

MUL tempTexCoord2.x, tempTexCoord.x, program.local[1].y;
MUL tempTexCoord2.y, tempTexCoord.y, program.local[1].x;
SUB colorTexCoordFinal.x, tempTexCoord2.x, tempTexCoord2.y;

MUL tempTexCoord2.x, tempTexCoord.y, program.local[1].y;
MUL tempTexCoord2.y, tempTexCoord.x, program.local[1].x;
ADD colorTexCoordFinal.y, tempTexCoord2.x, tempTexCoord2.y;

ADD colorTexCoordFinal.xy, colorTexCoordFinal, 0.5;

# scale the texture
MUL result.texcoord[1].xy, colorTexCoordFinal, program.local[0];

#######################################################

# rotate the alpha source texcoords
SUB tempTexCoord, vertex.texcoord[0], 0.5;

MUL tempTexCoord2.x, tempTexCoord.x, program.local[1].w;
MUL tempTexCoord2.y, tempTexCoord.y, program.local[1].z;
SUB colorTexCoordFinal.x, tempTexCoord2.x, tempTexCoord2.y;

MUL tempTexCoord2.x, tempTexCoord.y, program.local[1].w;
MUL tempTexCoord2.y, tempTexCoord.x, program.local[1].z;
ADD colorTexCoordFinal.y, tempTexCoord2.x, tempTexCoord2.y;

ADD colorTexCoordFinal.xy, colorTexCoordFinal, 0.5;

MOV result.texcoord[0], colorTexCoordFinal;

#######################################################

# calculate dest image texture coordinates
MOV result.texcoord[2], vertex.texcoord[2];

# transfer vertex color
MOV result.color, vertex.color;
