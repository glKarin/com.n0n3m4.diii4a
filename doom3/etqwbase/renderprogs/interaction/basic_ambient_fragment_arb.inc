
OPTION ARB_precision_hint_fastest;

TEMP	R0, R1, color, localNormal, worldNormal, ambient, localAmbient, specLook, diffuse;

PARAM	subOne = { -1, -1, -1, -1 };
PARAM	scaleTwo = { 2, 2, 2, 2 };

$ifdef parallax
	TEMP height;
	TEMP viewofs;
	TEMP _diffTC;
	TEMP _bumpTC;
	TEX	 height.x, fragment.texcoord[4], $heightMap, 2D;
#	TEX	 height.x, fragment.texcoord[4], $diffuseMap, 2D;
	MUL  height.x, height.x, -0.0015;
	MUL viewofs.xyz, fragment.texcoord[0], fragment.texcoord[0];
	RSQ viewofs.w, viewofs.w;
	MUL viewofs.xyz, viewofs.w, fragment.texcoord[0];
	MAD _diffTC, viewofs, height.x, fragment.texcoord[4];
	MAD _bumpTC, viewofs, height.x, fragment.texcoord[1];
$else
	ATTRIB _diffTC = fragment.texcoord[4];
	ATTRIB _bumpTC = fragment.texcoord[1];
$endif


$ifdef enable_detail
TEMP DTLCOORD;
MAD DTLCOORD.xy, _bumpTC, $detailMult, $detailMult.zwzw;
$endif

$ifdef enable_instdither
MUL R1.xy, fragment.position, 0.0625;
TEX R1, R1, $noiseMap, 2D;
SLT R1.w, fragment.texcoord[6].w, R1.w;
KIL	-R1.w;
$endif


# coverage dither mask
$ifdef r_useDitherMask
MUL R1.xy, fragment.position, 0.0625;
TEX	R1, R1, $noiseMap, 2D;
SLT R1.w, $coverage, R1.w;
KIL	-R1.w;
#MUL		R1.xy, fragment.position, 0.015625;
#MUL R1.xy, fragment.position, 0.0625;
#TEX		R0.r, R1, $noiseMap, 2D;
#SUB		R0.a, 0.5, R0.r;
#KIL		R0.a;
$endif

# modulate by the diffuse map and constant diffuse factor
TEX		diffuse, _diffTC, $diffuseMap, 2D;
$ifdef alphatest_kill
SUB		diffuse.a, diffuse.a, $alphaThresh;
KIL		diffuse.a;
$endif
$ifdef alphatest_alphatocoverage
SUB		R0.a, diffuse.a, 0.1;
KIL		R0.a;
$endif


$ifdef enable_detailweight_const
TEMP	weight;
MOV weight, 1;
$endif
$ifdef enable_detailweight_vtxalpha
TEMP	weight;
MOV		weight, fragment.color.a;
$endif
$ifdef enable_detailweight_diffalpha
TEMP	weight;
MOV		weight, diffuse.a;
$endif
$ifdef enable_detailweight_tex
TEMP	weight;
TEX		weight, _diffTC, $detailWeightMap, 2D;
$endif
$ifdef enable_detail
MUL_SAT		weight, weight, fragment.texcoord[1].w;
$endif

TEX		localNormal, _bumpTC, $bumpMap, 2D;
$if !r_dxnNormalMaps
MOV		localNormal.x, localNormal.a;
$endif
MAD		localNormal.xy, localNormal, scaleTwo, subOne;

$ifdef enable_bumpdetail
TEMP   localDetailNormal;
TEX localDetailNormal, DTLCOORD, $bumpDetailMap, 2D;
MOV localDetailNormal.x, localDetailNormal.a;
MAD	localDetailNormal.xy, localDetailNormal, scaleTwo, subOne;
MAD localNormal.xy, weight.x, localDetailNormal, localNormal;
$endif

#Renormalize
$if r_normalizeNormalMaps
MOV		localNormal.z, 0;
DP3		R1.x, localNormal,localNormal;
ADD		R1.x, 1, -R1.x;
RSQ		R1.x, R1.x;
RCP		localNormal.z, R1.x;
$endif

# put normal into world space
DP3		worldNormal.x, fragment.texcoord[5], localNormal;
DP3		worldNormal.y, fragment.texcoord[6], localNormal;
DP3		worldNormal.z, fragment.texcoord[7], localNormal;

# get diffuse lighting from cubemap
TEX		ambient, worldNormal, $ambientCubeMap, CUBE;
MUL		ambient, ambient, $ambientBrightness;


$ifdef enable_diffdetail
TEMP	detailDiff;
TEX		detailDiff, DTLCOORD, $diffuseDetailMap, 2D;
ADD		detailDiff, detailDiff, -0.5;
MAD		diffuse, weight.x, detailDiff, diffuse;
$endif

MUL		color, diffuse, $diffuseColor;
# kill if we need this

# modulate by ambient
MUL		color, ambient, color;

$if !r_shaderSkipSpecCubeMaps
# calc reflection vector: i - 2 * dot(i, n) * n
DP3		R1, fragment.texcoord[2], worldNormal;
ADD		R1, R1, R1;
MAD		R0, -worldNormal, R1.x, fragment.texcoord[2];

#DP3		ambient.a, fragment.texcoord[2], fragment.texcoord[2];
#RSQ		ambient.a, ambient.a;
#MUL		ambient, fragment.texcoord[2], ambient.a;
#MAD		ambient, ambient, 0.5, 0.5;

# get specular from cubemap
TEX		ambient, R0, $specularCubeMap, CUBE;

# modulate by the specular map * 2
TEX		specLook, _diffTC, $specularMap, 2D;

$ifdef enable_specdetail
TEMP		dtlspecLook;
TEX		dtlspecLook, DTLCOORD, $specDetailMap, 2D;
ADD		dtlspecLook, dtlspecLook, -0.5;
MAD_SAT		specLook, weight.x, dtlspecLook, specLook;
$endif
ADD		R0, specLook, specLook;

MAD		color, R0, ambient, color;
$endif

# multiply with ambient brightness
MUL		color, $ambientScale.x, color;

# do some special effects if we're compiling certain shaders
# we could move this to a separate file but this saves us from
# duplicating the shader

$ifdef self_illum
# Simple self illumination
TEX		R0, _diffTC, $selfIllumMap, 2D;
$ifdef self_illum_glow
MAD_SAT R0.rgb, R0.a, $parameters2, R0;
$endif

$ifdef self_illum_modulate
MAD		color, $parameters, R0, color;
$else
ADD		color, color, R0;
$endif
$endif

$ifdef strogg_effects
# "Beetle shell" empirical shader
DP3		R1, fragment.texcoord[2], worldNormal;
TEX		R0, R1, $stroggShellMap, 2D;
MAD		R1,	specLook, 1.8, 0.05;
MUL		R0, R1, R0;
MUL		R0, $sunColor, R0; #so it goes dimmer at night
ADD		color, color, R0;
$endif

# modify by the vertex color

$if ( r_megaDrawMethod != 0 )
LRP color.rgb, fragment.color.secondary.x, $fogColor, color;
$endif


MUL		result.color, color, fragment.color;
$ifdef alphatest_alphatocoverage
MUL		result.color.a, diffuse.a, 1.5;
$else
MOV		result.color.a, $ambientScale.y;
$endif

$ifdef enable_detail
#MOV		result.color.rgb, fragment.texcoord[7].w;
$endif

#MAD		result.color.rgb, worldNormal, 0.5, 0.5;
