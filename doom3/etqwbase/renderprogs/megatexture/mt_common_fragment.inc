
PARAM	defaultTexCoord = { 0, 0.5, 0, 1 };

TEMP	mask1, invMask1, R2, combined, scaled;
TEMP	R1, R3, R4, R5, R6, R7;

PARAM	clampConstants = { -32, 16, 0, 0 }; # Used to calculate masks

# load masks
$if ( r_megatexturePreferALU > 0 )
	# Use maths to calculate masks
	# This is the preferred solution for recent graphic HW which boasts higher ALUs than TEX instructions
		
	# Calculate 0.5-s and 0.5-t for all s and t coordinates
	SUB		R1, 0.5, fragment.texcoord[0].xyzw;
	SUB		R2, 0.5, fragment.texcoord[1].xyzw;
	
	# Compute absolute value of all (0.5-s) and (0.5-t)
	ABS		R1, R1;
	ABS		R2, R2;
	
	# Find maximum between each s and t pair
	MAX		R1.xy, R1.xzwy, R1.ywxz;
	MAX		R1.zw, R2.zwxz, R2.xyyw;
	
	# Calculate masks
	MAD_SAT	mask1.yw, R1.zxwy, clampConstants.xxxx, clampConstants.yyyy;
	$if ( r_shaderQuality < 2 )
		MAD_SAT	mask1.x, R1.zxwy, clampConstants.xxxx, clampConstants.yyyy;
	$endif	
	$if ( r_shaderQuality < 1 )
		MAD_SAT	mask1.z, R1.zxwy, clampConstants.xxxx, clampConstants.yyyy;
	$endif

$else
	TEX		mask1.y, fragment.texcoord[0].xyxy, $borderClampMap, 2D;
	TEX		mask1.w, fragment.texcoord[0].zwzw, $borderClampMap, 2D;
	$if ( r_shaderQuality < 2 )
		TEX		mask1.x, fragment.texcoord[1].xyxy, $borderClampMap, 2D;
	$endif	
	$if ( r_shaderQuality < 1 )
		TEX		mask1.z, fragment.texcoord[1].zwzw, $borderClampMap, 2D;
	$endif
$endif

# apply level opacities
MUL		mask1, mask1, $megaTextureLevelOpacity_1_5;

# calculate inverse mask
SUB		invMask1, 1, mask1;

# load the minimum quality image first, which will be used
# if none of the other textures have a valid mask
MOV		combined, 0;

MOV		scaled, fragment.texcoord[7];

# load the multi-resolution textures and sum up with the masks
$if ( r_megatextureDisableFilteringOptimization == 1 )
	$if ( r_shaderQuality < 2 )
		MUL		R2.xy, scaled, 2;
	$endif
	MUL		R3.xy, scaled, 4;
	$if ( r_shaderQuality < 1 )
		MUL		R4.xy, scaled, 8;
	$endif
	MUL		R5.xy, scaled, 16;
	TEX		R1, scaled, $megaTextureLevel_0, 2D;  # lowest quality moving image
$else
	PARAM	saturatedScale = { 2, 4, 8, 16 };
	TEMP	saturated, satscale;

	MUL_SAT saturated, mask1, 100000;
	MUL		satscale, saturatedScale, saturated;

	$if ( r_shaderQuality < 2 )
		MUL_SAT saturated.x, invMask1.x, 100000; 
		MUL		R1.xy, saturated.x, scaled;
		MUL		R2.xy, scaled, satscale.x;
	$else
		MUL_SAT saturated.y, invMask1.y, 100000; 
		MUL		R1.xy, saturated.y, scaled;
	$endif
	MUL		R3.xy, scaled, satscale.y;
	$if ( r_shaderQuality < 1 )
		MUL		R4.xy, scaled, satscale.z;
	$endif
	MUL		R5.xy, scaled, satscale.w;
	TEX		R1, R1, $megaTextureLevel_0, 2D;  # lowest quality moving image
	
$endif


$if ( r_shaderQuality < 2 )
	TEX		R2, R2, $megaTextureLevel_1, 2D;
$endif
TEX		R3, R3, $megaTextureLevel_2, 2D;
$if ( r_shaderQuality < 1 )
	TEX		R4, R4, $megaTextureLevel_3, 2D;
$endif
TEX		R5, R5, $megaTextureLevel_4, 2D;

MOV		combined, R1;
$if ( r_shaderQuality < 2 )
	MUL		combined, combined, invMask1.x;
	MAD		combined, R2, mask1.x, combined;
$endif
MUL		combined, combined, invMask1.y;
MAD		combined, R3, mask1.y, combined;
$if ( r_shaderQuality < 1 )
	MUL		combined, combined, invMask1.z;
	MAD		combined, R4, mask1.z, combined;
$endif
MUL		combined, combined, invMask1.w;
MAD		combined, R5, mask1.w, combined;


# detail texture
$include "megatexture/detail_fragment.inc"
