TEMP dest, cover, stamp;
TEMP stampTexCoord, normal, destNormal, scalar;
TEMP R0;

TEX R0, fragment.texcoord[0], texture[0], 2D;
$ifndef megagen
MOV dest, R0.agbr;
$else
MOV dest, R0;
$endif

TEX cover, fragment.texcoord[1], texture[1], 2D;

CMP stampTexCoord, program.local[2].x, fragment.texcoord[0], fragment.texcoord[1];

TEX stamp, stampTexCoord, texture[3], 2D;

MOV scalar.x, cover;
ADD scalar.y, 1, -cover.x;

# decode normal (normalize?)
MAD R0, stamp, 2, -1;

# rotate normal
DP3 normal.x, R0, program.local[0];
DP3 normal.y, R0, program.local[1];
MOV normal.z, R0.z;

# decode destination
MAD destNormal, dest, 2, -1;

# flatten normals
RCP R0, normal.z;
MUL scalar.x, scalar, R0;
MUL normal.xy, scalar.x, normal;
MOV normal.z, 1;

RCP R0, destNormal.z;
MUL scalar.y, scalar, R0;
MUL destNormal.xy, scalar.y, destNormal;

# add together
ADD normal.xy, normal, destNormal;

# normalize
DP3	R0, normal, normal;
RSQ	R0, R0.x;
MUL	R0, R0.x, normal;

# encode normals
MAD	R0.rgb, R0, .5, .5;
MOV R0.a, 1;

$ifndef megagen
# swizzle components
MOV result.color, R0.agbr;
$else
MOV result.color, R0;
$endif
