OPTION ARB_precision_hint_fastest;

TEMP	R0, R1, color, worldNormal, ambient, localAmbient, specLook, diffuse;

ATTRIB _diffTC = fragment.texcoord[4];
# modulate by the diffuse map and constant diffuse factor
TEX		diffuse, _diffTC, $diffuseMap, 2D;
$ifdef alphatest_kill
SUB		diffuse.a, diffuse.a, $alphaThresh;
KIL		diffuse.a;
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
#TEX		R1.r, R1, $dithermask, 2D;
#SUB		R1.a, 0.5, R1.r;
#KIL		R1.a;
$endif

# put normal into world space
MOV		worldNormal, fragment.texcoord[5];

# get diffuse lighting from cubemap
TEX		ambient, worldNormal, $ambientCubeMap, CUBE;
MUL		ambient, ambient, $ambientBrightness;
MUL		color, diffuse, $diffuseColor;
# modulate by ambient
MUL		color, ambient, color;

$ifdef self_illum
# Simple self illumination
TEX		R1, _diffTC, $selfIllumMap, 2D;
$ifdef self_illum_glow
MAD_SAT R1.rgb, R1.a, $parameters2, R1;
$endif
ADD color, R1, color;
$endif

# multiply with ambient brightness
MUL		color, $ambientScale.x, color;

$if ( r_megaDrawMethod != 0 )
LRP color.rgb, fragment.color.secondary.x, $fogColor, color;
$endif

MUL		result.color, color, fragment.color;
MOV		result.color.a, $ambientScale.y;

