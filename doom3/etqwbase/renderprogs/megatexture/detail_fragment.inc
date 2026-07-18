$if r_detailTexture
TEMP	DTR1, DTR2;

TEX		DTR1, fragment.texcoord[7].zwzz, $megaDetailTexture, 2D;
MAD		R1, DTR1, 2, -1;
TEX		DTR2, fragment.texcoord[7], $megaDetailTextureMask, 2D;
DP4		R1, DTR2, R1;
MAD		combined, R1, combined, combined;

$ifdef SNOWBLUING
PARAM BLUE = {-0.2,-0.08,0.,0.};
TEMP DTR0;
SUB	R1.rgb, 1, BLUE;
MUL DTR0, fragment.color.secondary, DTR2.z;
MAD combined.rgb, BLUE, DTR0.x, combined;
$endif

#MOV combined.rgb, fragment.color.secondary;
$endif