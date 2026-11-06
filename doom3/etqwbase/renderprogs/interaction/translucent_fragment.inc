OPTION ARB_precision_hint_fastest;

TEMP	light, color, R1, R2, localNormal, specular, faceSign;

PARAM	subOne = { -1, -1, -1, -1 };
PARAM	scaleTwo = { 2, 2, 2, 2 };
PARAM	grayscale = { 0.3, 0.59, 0.11, 0.0};

$if !r_usePerFragmentSpecular
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

$if r_usePerFragmentSpecular
	# calculate the half angle vector in object space
	ADD	specular, view, light;

	# normalize
	DP3	R1.x, specular, specular;
	RSQ	R1.x, R1.x;
	MUL	specular, R1.x, specular;
$endif

TEX	localNormal, fragment.texcoord[1], $bumpMap, 2D;
$if !r_dxnNormalMaps
MOV localNormal.x, localNormal.a;
$endif
MAD	localNormal, localNormal, scaleTwo, subOne;

$if r_normalizeNormalMaps
MOV		localNormal.z, 0;
DP3		R1.x, localNormal,localNormal;
ADD		R1.x, 1, -R1.x;
RSQ		R1.x, R1.x;
RCP		localNormal.z, R1.x;
$endif


# store face sign
MOV faceSign, light.z;

# if - back facing
TEMP backLight, frontLight;


TEX R2, fragment.texcoord[4], $diffuseMap, 2D;
TEMP diffcolor;
MOV diffcolor, R2;

$ifdef alphatest
$ifdef amblit
SUB R1.a, R2.a, $alphaThresh;
KIL R1.a;
$endif
# extract translucency from upper half of diffuse map alpha
#MAD R2.a, R2.a, scaleTwo, subOne;
MAD R2.a, R2.a, 1.25, -0.25;
$endif

DP3 backLight.x, light, -localNormal;
MUL backLight.x, backLight.x, R2.a;

# modulate by the translucency coefficient
MUL backLight.x, backLight.x, $parameters.a;

# else - front facing
DP3 frontLight.x, light, localNormal;

# choose which one to use for this fragment
CMP light.x, faceSign.x, backLight.x, frontLight.x;

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
MUL	color, R2, $diffuseColor;

# modulate by leaf 'subsurface scattering' colour
MUL R1.rgb, color, $parameters;
CMP color.rgb, faceSign.x, R1, color;

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

# disable specular when rendering back face
CMP R1, faceSign.x, 0, R1;

# modulate by the specular map * 2
TEX	R2, fragment.texcoord[5], $specularMap, 2D;
ADD	R2, R2, R2;
MAD	color, R1, R2, color;

MUL	color, light, color;

# modify by the vertex color

$ifdef ambient
MAD color, diffcolor, $ambientAvgColor, color;
$endif

$if ( r_megaDrawMethod != 0 )
$ifdef amblit
LRP result.color.rgb, fragment.color.secondary.x, $fogColor, color;
$else
SUB R1.x, 1, fragment.color.secondary.x;
MUL result.color.rgb, color, R1.x;
$endif
$else
MOV result.color.rgb, color;
$endif

MUL R1, R1, R2;
MUL R1, R1, light;
DP3 R1.x, R1, grayscale;

# full bloom on back face
CMP result.color.a, faceSign.x, 1, R1.x;
