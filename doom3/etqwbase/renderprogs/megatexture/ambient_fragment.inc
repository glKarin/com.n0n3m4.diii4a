OPTION ARB_precision_hint_fastest;

TEMP	R8;

$include "megatexture/mt_common_fragment.inc"

TEMP SUNDP;
# Sample ambient cube map using texcoord unpacked from color register
DP3_SAT	SUNDP, fragment.texcoord[5], $sunDirectionWorld;
TEX		R7, R7, $ambientCubeMap, CUBE;
MUL		R7, R7, $ambientBrightness;

$if r_skipDiffuse
	MOV		combined.rgb, 1;
$endif

$ifndef premult
	$if !r_skipBump
		SUB SUNDP, combined.a, SUNDP;
		ADD SUNDP, SUNDP, 1;
		MAX SUNDP, SUNDP, 0;
		MUL		combined, combined, SUNDP;
	$endif
$endif

$ifdef bakedamb
	TEMP LIGHTINFO;
	MUL LIGHTINFO, SUNDP, combined;
	MUL LIGHTINFO, LIGHTINFO, $parameters.z; #0.75;
	SUB_SAT		combined, combined, LIGHTINFO;
$else
	MUL combined, combined, R7;
$endif

#final stuffs
$ifdef brightness
MUL		combined.xyz, combined, $parameters.x;
$endif

$ifdef megatexSpecular
MOV		R2.xyz, $sunDirection;

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

TEMP	SPECMASK;
TEX		SPECMASK, fragment.texcoord[7], $mask, 2D;

MUL		R1.w, R1.w, SPECMASK.x;

ADD		R1.w, 1, R1.wwww;
MUL		combined, combined, $ambientScale.x;
MUL		result.color.xyz, combined, R1.w;
$else
MUL		result.color.xyz, combined, $ambientScale.x;
$endif
MOV		result.color.a, $ambientScale.z;
