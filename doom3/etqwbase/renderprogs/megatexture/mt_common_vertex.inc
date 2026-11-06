# masks have a scale and offset
# Store each texture coordinate into its individual array
MAD		result.texcoord[0].xy, $texCoordAttrib.xyxy, $megaMaskParams_2.w, $megaMaskParams_2.xyxy;
MAD		result.texcoord[0].zw, $texCoordAttrib.xyxy, $megaMaskParams_4.w, $megaMaskParams_4.xyxy;
$if ( r_shaderQuality < 2 )		
	MAD		result.texcoord[1].xy, $texCoordAttrib.xyxy, $megaMaskParams_1.w, $megaMaskParams_1.xyxy;
$endif
$if ( r_shaderQuality < 1 )		
	MAD		result.texcoord[1].zw, $texCoordAttrib.xyxy, $megaMaskParams_3.w, $megaMaskParams_3.xyxy;
$endif

$ifdef megatexSpecular
SUB result.texcoord[3], $lightOrigin, $positionAttrib;
SUB result.texcoord[4], $viewOrigin, $positionAttrib;
$endif
MOV result.texcoord[5], $normalAttrib;

# images just have a scale
MOV		result.texcoord[7], $texCoordAttrib;

$include "megatexture/detail_vertex.inc"
