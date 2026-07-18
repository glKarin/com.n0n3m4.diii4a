		uniform sampler2D 	$megaTextureLevel_0;
		uniform sampler2D 	$megaTextureLevel_1;
		uniform sampler2D 	$megaTextureLevel_2;
		uniform sampler2D 	$megaTextureLevel_3;
		uniform sampler2D 	$megaTextureLevel_4;
		uniform sampler2D 	$megaTextureLevel_5;
		uniform sampler2D 	$megaDetailTexture;
		uniform sampler2D 	$megaDetailTextureMask;
		uniform samplerCube $ambientCubeMap;
		
		uniform vec4 		$megaTextureParams_0;
		uniform vec4 		$megaTextureParams_1;
		uniform vec4 		$megaTextureParams_2;
		uniform vec4 		$megaTextureParams_3;
		uniform vec4 		$megaTextureParams_4;
		uniform vec4 		$megaTextureParams_5;
		uniform vec4 		$diffuseColor;
		uniform vec4 		$ambientScale;
		uniform vec4		$sunDirectionWorld;
		uniform vec4        $parameters;
		
		varying	vec4 		texcoord3;
		varying	vec4 		texcoord4;
		varying	vec3 		texcoord6;
		varying	vec4 		texcoord7;
		

		void main (void)
		{
			vec4 combined;
			
			// GLSL fragment shader: code for fetching megatextures
			$include "megatexture/common_glsl_fragment.inc"
			
			//compute the sun dot product
			float sunDP = clamp( dot( texcoord6.xyz, $sunDirectionWorld.xyz), 0.0, 1.0);
			
			$if r_megaMultiply
				vec4 R7 = texture2D(texture[11], texcoord7) * program.local[11].y;
				R7 = clamp(R7 * combined, 0.0, 1.0);
				combined.xyz = mix(R7, combined, program.local[11].x);
			$endif
			
			$if r_skipDiffuse
				combined.xyz = vec4(1.0);
			$endif
			
			
//$ifndef premult
//$if !r_skipBump
//
//SUB SUNDP, combined.a, SUNDP;
//ADD SUNDP, SUNDP, 1;
//MAX SUNDP, SUNDP, 0;
//MUL		combined, combined, SUNDP;
//$endif
//$endif

			$ifndef premult
			$if !r_skipBump
				sunDP = max( 0.0, 1.0 + combined.a - sunDP);
				combined *= sunDP;
			$endif
			$endif
			
//$ifdef bakedamb
//TEMP LIGHTINFO;
//MUL LIGHTINFO, SUNDP, combined;
//MUL LIGHTINFO, LIGHTINFO, $parameters.z; #0.75;
//SUB_SAT		combined, combined, LIGHTINFO;
//$else
//MUL combined, combined, R7;
//$endif
			
			$ifdef bakedamb
			combined = clamp( combined - sunDP * $parameters.z * combined, 0.0, 1.0);
			$else
			// Sample ambient cube map using texcoord
			combined *= textureCube($ambientCubeMap, texcoord6);
			$endif
			
//# detail texture
//$ifndef fallback
//$if r_detailTexture
//include "megatexture/detail_fragment.inc"
//$endif
//$endif
			
			// detail texture
			$if r_detailTexture
				R1 = texture2D($megaDetailTexture, texcoord7.zw);
				R1 = R1*2.0 - 1.0;
				R2 = texture2D($megaDetailTextureMask, texcoord7.xy);
				combined += combined*dot(R2, R1);
			$endif
			
//#final stuffs
//$ifdef brightness
//MUL		combined.xyz, combined, $parameters.x;
//$endif
//MUL		result.color, combined, $ambientScale.x;
//MOV		result.color.a, $ambientScale.z;

			// final stuffs
			$ifdef brightness
			combined.rgb *= $parameters.x;
			$endif
			gl_FragColor = vec4(combined.rgb * $ambientScale.x, $ambientScale.z);
		}