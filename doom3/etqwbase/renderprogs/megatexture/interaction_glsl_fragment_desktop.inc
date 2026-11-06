
		uniform sampler2D 	$lightFalloffMap;
		uniform sampler2D 	$lightProjectionMap;
		uniform sampler2D 	$megaTextureLevel_0;
		uniform sampler2D 	$megaTextureLevel_1;
		uniform sampler2D 	$megaTextureLevel_2;
		uniform sampler2D 	$megaTextureLevel_3;
		uniform sampler2D 	$megaTextureLevel_4;
		uniform sampler2D 	$megaTextureLevel_5;
		uniform sampler2D 	$megaDetailTexture;
		uniform sampler2D 	$megaDetailTextureMask;
		
		uniform vec4 		$megaTextureParams_0;
		uniform vec4 		$megaTextureParams_1;
		uniform vec4 		$megaTextureParams_2;
		uniform vec4 		$megaTextureParams_3;
		uniform vec4 		$megaTextureParams_4;
		uniform vec4 		$megaTextureParams_5;
		uniform vec4 		$diffuseColor;
		uniform vec4        $parameters;
		
		const vec4 defaultTexCoord = vec4( 0.0, 0.5, 0.0, 1.0 );

		varying	vec4 		texcoord3;
		varying	vec4 		texcoord4;
		varying	vec4 		texcoord6;
		varying	vec4 		texcoord7;
		varying	vec4 		vertexColor;

		void main (void)
		{
			vec4 combined;
			
			// GLSL fragment shader: code for fetching megatextures
			$include "megatexture/common_glsl_fragment.inc"
			
//$ifdef bakedamb
//MUL combined.xyz, combined, $parameters.w;
//$endif

			$ifdef bakedamb
			combined.rgb *= $parameters.w;
			$endif
	
			$if r_megaMultiply
				vec4 R7 = texture2D(texture[11], texcoord7) * program.local[11].y;
				R7 = clamp(R7 * combined, 0.0, 1.0);
				combined.xyz = mix(R7, combined, program.local[11].x);
			$endif

			$if r_skipDiffuse
				combined.xyz = $diffuseColor.rgb;
			$endif

//$ifndef premult
//$if !r_skipBump
//MUL		combined, combined, combined.a;
//$endif
//$endif

			$ifndef premult
			$if !r_skipBump
				combined *= combined.a;
			$endif
			$endif

			// calc falloff
			combined *= texture2D($lightFalloffMap, vec2(texcoord6.z, 0.5));
			
			combined *= texture2D($lightProjectionMap, texcoord6.xy);

			// detail texture
			$if r_detailTexture
				R1 = texture2D($megaDetailTexture, texcoord7.zw);
				R1 = R1*2.0 - 1.0;
				R2 = texture2D($megaDetailTextureMask, texcoord7.xy);
				combined += combined*dot(R2, R1);
			$endif
			
//$ifdef brightness
//MUL		combined.xyz, combined, $parameters.y;
//$endif

			$ifdef brightness
			combined.rgb *= $parameters.y;
			$endif

			gl_FragColor = vec4(combined.xyz * vertexColor.xyz, 0.0);
		}