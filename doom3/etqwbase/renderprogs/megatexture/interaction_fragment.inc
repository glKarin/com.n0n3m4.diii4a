OPTION ARB_precision_hint_fastest;

$include "megatexture/mt_common_fragment.inc"

MUL		combined.xyz, combined, $diffuseColor;

$ifdef bakedamb
	MUL combined.xyz, combined, $parameters.w;
$endif

$if r_skipDiffuse
	MUL		combined.rgb, 1, $diffuseColor;
$endif

$ifndef premult
	$if !r_skipBump
		MUL		combined, combined, combined.a;
	$endif
$endif

# calc falloff
MOV		R7, defaultTexCoord;
MOV		R7.x, fragment.texcoord[6].z;
TEX		R7, R7, $lightFalloffMap, 2D;
MUL		combined, combined, R7;

TXP		R7, fragment.texcoord[6], $lightProjectionMap, 2D;
MUL		combined, combined, R7;

$ifdef brightness
	MUL		combined.xyz, combined, $parameters.y;
$endif

#MUL		result.color.xyz, combined, fragment.color;
MOV		result.color.a, 0;

$ifdef megatexSpecular
	MUL		combined, combined, fragment.color;
	MAD		result.color.xyz, fragment.texcoord[4], 0.00005, 0.5;
	DP3		R1.w, fragment.texcoord[3], fragment.texcoord[3];
	RSQ		R1.w, R1.w;
	MUL		R2.xyz, fragment.texcoord[3], R1.w;

	DP3		R1.w, fragment.texcoord[4], fragment.texcoord[4];
	RSQ		R1.w, R1.w;
	MUL		R3.xyz, fragment.texcoord[4], R1.w;

	ADD		R1, R2, R3;
	DP3		R1.w, R1, R1;
	RSQ		R1.w, R1.w;
	MUL		R1.xyz, R1, R1.w;
	DP3		R1.w, R1, fragment.texcoord[5];
	MUL		R1.w, R1.w, R1.w;
	MUL		R1.w, R1.w, R1.w;
#	MUL		R1.w, R1.w, R1.w;
#	MUL		R1.w, R1.w, R1.w;
	
	TEMP	SPECMASK;
	TEX		SPECMASK, fragment.texcoord[7], $mask, 2D;
	
	MUL		R1.w, R1.w, 20;
	MUL		R1.w, R1.w, SPECMASK.x;
	
	ADD		R1.w, 1, R1.wwww;
	MUL		result.color.xyz, combined, R1.w;
	#MOV		result.color.xyz, SPECMASK;
	#MOV		result.color.xyz, R1.w;
$else
	MUL		result.color.xyz, combined, fragment.color;
$endif
#MOV		result.color.xyz, combined.a;
