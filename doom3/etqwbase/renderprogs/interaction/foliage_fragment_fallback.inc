OPTION ARB_precision_hint_fastest;

TEMP	light, color, R1, R2, localNormal, specular;

# modulate by the light projection
TXP	light, fragment.texcoord[3], $lightProjectionMap, 2D;


$ifdef enable_instdither
MUL R1.xy, fragment.position, 0.0625;
TEX R1, R1, $noiseMap, 2D;
SLT R1.w, fragment.texcoord[6].w, R1.w;
KIL	-R1.w;
$endif

# modulate by the light falloff
TEX	R1, fragment.texcoord[2], $lightFalloffMap, 2D;
MUL	light, light, R1;

# modulate by the diffuse map and constant diffuse factor
TEX	R1, fragment.texcoord[4], $diffuseMap, 2D;
MUL R1.rgb, R1, fragment.color;



MUL	color, R1, $diffuseColor;
$ifdef alphatest_kill
TEMP	_AR0;
SUB		_AR0.a, R1.a, $alphaThresh;
KIL		_AR0.a;
$endif
$ifdef alphatocoverage
TEMP	_AR0;
SUB		_AR0.a, R1.a, $alphaThresh;
KIL		_AR0.a;
$endif


MUL	color, light, color;
$ifdef ambient
MAD color, R1, $ambientAvgColor, color;
$endif

# modify by the vertex color

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

$ifdef alphatocoverage
MUL result.color.a, color.a, 1.5;
$else
MOV result.color.a, 1;
$endif
