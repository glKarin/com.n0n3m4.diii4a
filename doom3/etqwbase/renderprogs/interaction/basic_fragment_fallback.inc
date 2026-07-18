OPTION ARB_precision_hint_fastest;

TEMP	light, color, R1, R2, specular, diffuse, specLook;

PARAM	grayscale = { 0.3, 0.59, 0.11, 0.0 };
ATTRIB _diffTC = fragment.texcoord[4];
ATTRIB _bumpTC = fragment.texcoord[1];

# modulate by the diffuse map and constant diffuse factor
TEX		diffuse, _diffTC, $diffuseMap, 2D;


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

TEMP blah;
MOV blah, R1;

$ifdef alphatest_kill
SUB		R1.a, diffuse.a, $alphaThresh;
KIL		R1.a;
$endif


# modulate by the light projection
TXP	R1, fragment.texcoord[3], $lightProjectionMap, 2D;
MUL	light, fragment.texcoord[0].z, R1;
MAX light, light, 0;

# modulate by the light falloff
TEX	R1.x, fragment.texcoord[2], $lightFalloffMap, 2D;
MUL	light, light, R1.x;

# modify by the vertex color
TEMP diffcolor;
MUL diffcolor, diffuse, fragment.color;

MUL	color, diffcolor, $diffuseColor;
TEMP _finalcolor;
MUL	_finalcolor, light, color;

$ifdef addambient
MAD _finalcolor, diffcolor, $ambientAvgColor, _finalcolor;
$endif

$ifdef self_illum
# Simple self illumination
TEX		R1, _diffTC, $selfIllumMap, 2D;
$ifdef self_illum_glow
MAD_SAT R1.rgb, R1.a, $parameters2, R1;
$endif
ADD _finalcolor, R1, _finalcolor;
$endif

MOV result.color.a, 0;#fragment.color.secondary.x;

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

