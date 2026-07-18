OPTION ARB_position_invariant;

TEMP	R0, R1;

$include "megatexture/mt_common_vertex.inc"

DP4		R0.x, $positionAttrib, $lightProject_s;
DP4		R0.y, $positionAttrib, $lightProject_t;
DP4		R0.z, $positionAttrib, $lightFalloff_s;
DP4		R0.w, $positionAttrib, $lightProject_q;
MOV		result.texcoord[6], R0;

# calculate vector to light in R0 (object space)
SUB		R0, $lightOrigin, $positionAttrib;
DP3		R1, R0, R0;
RSQ		R1, R1.x;
MUL		R0, R0, R1.x;

DP3		R0, $normalAttrib, R0;
MAX		R0, R0, 0;
ADD		R1, 1, -R0;
MAD		result.color, $megaBlendOutDotP.x, R1, R0;

TEMP _R1, _R2;
SUB		_R2, $positionAttrib, $viewOrigin;
DP3		_R1.x, _R2, _R2;
RSQ		_R1.y, _R1.x;
MUL		_R1.x, _R1.y, _R1.x;
MUL		_R1.x, _R1.x, 0.00002;
#EX2		result.texcoord[3], -R1.x;
SUB		result.color.secondary, 1, _R1.x;
