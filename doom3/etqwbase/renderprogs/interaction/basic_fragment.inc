
$ifndef self_illum
OPTION ARB_precision_hint_fastest;
$endif

TEMP	light, color, R1, R2, localNormal, specular, diffuse, specLook;

PARAM	subOne = { -1, -1, -1, -1 };
PARAM	scaleTwo = { 2, 2, 2, 2 };
PARAM	grayscale = { 0.3, 0.59, 0.11, 0.0 };

$ifdef 	parallax
	TEMP height;
	TEMP viewofs;
	TEMP _diffTC;
	TEMP _bumpTC;
	TEX	 height.x, fragment.texcoord[4], $heightMap, 2D;
#	TEX	 height.x, fragment.texcoord[4], $diffuseMap, 2D;
	MUL  height.x, height.x, -0.0005;
	MUL viewofs.xyz, fragment.texcoord[7], fragment.texcoord[7];
	RSQ viewofs.w, viewofs.w;
	MUL viewofs.xyz, viewofs.w, fragment.texcoord[7];
	MAD _diffTC, viewofs, height.x, fragment.texcoord[4];
	MAD _bumpTC, viewofs, height.x, fragment.texcoord[1];
$else
	ATTRIB _diffTC = fragment.texcoord[4];
	ATTRIB _bumpTC = fragment.texcoord[1];
$endif

$ifdef enable_detail
TEMP DTLCOORD;
MAD DTLCOORD.xy,_bumpTC, $detailMult, $detailMult.zwzw;
$endif

$ifdef enable_instdither
MUL R1.xy, fragment.position, 0.0625;
TEX R1, R1, $noiseMap, 2D;
SLT R1.w, fragment.texcoord[6].w, R1.w;
KIL	-R1.w;
$endif

$ifdef enable_separate_depth
TEX		R1, fragment.texcoord[2].zwzw, $mask, 2D;
SUB		R1, R1.a, $parameters.x;
KIL		R1;
$endif

# modulate by the diffuse map and constant diffuse factor
TEX		diffuse, _diffTC, $diffuseMap, 2D;

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


# coverage dither mask
$ifdef r_useDitherMask
MUL R1.xy, fragment.position, 0.0625;
TEX	R1, R1, $noiseMap, 2D;
SLT R1.w, $coverage, R1.w;
KIL	-R1.w;
#TEMP _R1, _R0;
#MUL		_R1.xy, fragment.position, 0.015625;
#TEX		_R0.r, _R1, $dithermask, 2D;
#SUB		_R0.a, 0.5, _R0.r;
#KIL		_R0.a;
$endif

$if !r_usePerFragmentSpecular
	$if !r_shaderSkipSpecCubeMaps
		# load the specular half angle first, because
		# the ATI shader gives a "too many indirections" error
		# if this is done right before the texture indirection

		#-----------------
		#TEX	specular, fragment.texcoord[6], $normalCubeMap, CUBE;
		#MAD	specular, specular, scaleTwo, subOne;


		# instead of using the normalization cube map, normalize with math
		DP3	specular.x, fragment.texcoord[6], fragment.texcoord[6];
		RSQ	specular.x, specular.x;
		MUL	specular, specular.x, fragment.texcoord[6];
		#-----------------
	$endif
$else
	TEMP	view;
	
	# normalize view vector
	DP3	view.x, fragment.texcoord[6], fragment.texcoord[6];
	RSQ	view.x, view.x;
	MUL	view, view.x, fragment.texcoord[6];
$endif

#
# the amount of light contacting the fragment is the
# product of the two light projections and the surface
# bump mapping
#

# perform the diffuse bump mapping
$if !r_shaderPreferALU
	TEX	light, fragment.texcoord[0], $normalCubeMap, CUBE;
	MAD	light, light, scaleTwo, subOne;
$else
	# instead of using the normalization cube map, normalize with math
	DP3	light.x, fragment.texcoord[0], fragment.texcoord[0];
	RSQ	light.x, light.x;
	MUL	light, light.x, fragment.texcoord[0];
$endif

$if !r_shaderSkipSpecCubeMaps
	$if r_usePerFragmentSpecular
		# calculate the half angle vector in object space
		ADD	specular, view, light;

		# normalize
		DP3	R1.x, specular, specular;
		RSQ	R1.x, R1.x;
		MUL	specular, R1.x, specular;
	$endif
$endif

TEX	localNormal,_bumpTC, $bumpMap, 2D;
$if !r_dxnNormalMaps
MOV localNormal.x, localNormal.a;
$endif
MAD	localNormal, localNormal, scaleTwo, subOne;

$ifdef enable_bumpdetail
TEMP   localDetailNormal;
TEX localDetailNormal, DTLCOORD, $bumpDetailMap, 2D;
MOV localDetailNormal.x, localDetailNormal.a;
MAD	localDetailNormal.xy, localDetailNormal, scaleTwo, subOne;
MAD localNormal.xy, weight.x, localDetailNormal, localNormal;
$endif

$if r_normalizeNormalMaps
MOV		localNormal.z, 0;
DP3		R1.x, localNormal,localNormal;
ADD		R1.x, 1, -R1.x;
RSQ		R1.x, R1.x;
RCP		localNormal.z, R1.x;
$endif

DP3_SAT	light.x, light, localNormal;

# modulate by the light projection
TXP	R1, fragment.texcoord[3], $lightProjectionMap, 2D;
MUL	light, light.x, R1;

# modulate by the light falloff
TEX	R1.x, fragment.texcoord[2], $lightFalloffMap, 2D;
MUL	light, light, R1.x;

#
# the light will be modulated by the diffuse and
# specular surface characteristics
#

# modulate by the diffuse map and constant diffuse factor
$ifdef enable_diffdetail
TEMP    detailDiff;
TEX		detailDiff, DTLCOORD, $diffuseDetailMap, 2D;
ADD		detailDiff, detailDiff, -0.5;
MAD		diffuse, weight.x, detailDiff, diffuse;
$endif

$ifdef alphatest_kill
SUB		R1.a, diffuse.a, $alphaThresh;
KIL		R1.a;
$endif

MUL diffuse, diffuse, fragment.color;


MUL	color, diffuse, $diffuseColor;

$if !r_shaderSkipSpecCubeMaps
	# perform the specular bump mapping
	DP3	specular.x, specular, localNormal;

	$if !r_shaderPreferALU
		# perform a dependent table read for the specular falloff
		TEX	R1, specular, $specularTable, 2D;
	$else
		# Specular falloff: (sat(4x-3))^2 matches the function inside the specular table texture
		MAD_SAT R1.x, specular.x, 4.0, -3.0;
		MUL R1.x, R1.x, R1.x;
	$endif

	# modulate by the constant specular factor
	MUL	R1, R1.x, $specularColor;

	# modulate by the specular map * 2
	TEX	R2, fragment.texcoord[5], $specularMap, 2D;
	ADD	R2, R2, R2;
	$ifdef enable_specdetail
		TEX		specLook, DTLCOORD, $specDetailMap, 2D;
		ADD		specLook, specLook, -0.5;
		MAD		R2, weight.x, specLook, R2;
	$endif

	MAD	color, R1, R2, color;
$endif

TEMP _finalcolor;
MUL	_finalcolor.rgb, light, color;
$ifdef ambient
MAD _finalcolor.rgb, diffuse, $ambientAvgColor, _finalcolor;
$endif

$ifdef self_illum
# Simple self illumination
TEX		R1, _diffTC, $selfIllumMap, 2D;
$ifdef self_illum_glow
MAD_SAT R1.rgb, R1.a, $parameters2, R1;
$endif
ADD _finalcolor, R1, _finalcolor;
$endif

$ifdef strogg_effects
# "Beetle shell" empirical shader
DP3		R1, specular, localNormal;
TEX		R2, R1, $stroggShellMap, 2D;
MAD		R1,	R1, 0.2, 0.01;
MUL		R2, R1, R2;
MUL		R2, $sunColor, R2; #so it goes dimmer at night
ADD		_finalcolor, _finalcolor, R2;
$endif


$if ( r_megaDrawMethod != 0 )
$ifdef amblit
LRP result.color.rgb, fragment.color.secondary.x, $fogColor, _finalcolor;
$else
SUB R1.x, 1, fragment.color.secondary.x;
MUL result.color.rgb, _finalcolor, R1.x;
$endif
$else
MOV result.color.rgb, _finalcolor;
$endif

# modify by the vertex color


MUL R1, R1, R2;
MUL R1, R1, light;
DP3 result.color.a, R1, grayscale;

#	MUL viewofs.xyz, fragment.texcoord[7], fragment.texcoord[7];
#	RSQ viewofs.w, viewofs.w;
#	MUL viewofs.xyz, viewofs.w, fragment.texcoord[7];

#MAD result.color.rgb, viewofs, 0.5, 0.5;

#MOV		result.color.rgb, fragment.color.secondary;

#MOV result.color.a, 1;

