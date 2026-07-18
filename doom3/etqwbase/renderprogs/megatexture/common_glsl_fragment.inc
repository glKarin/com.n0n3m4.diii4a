
	//
	// GLSL code common to both megatexture GLSL ambient.rprog and interaction.rprog
	//

	vec4 R1, R2, R3, R4, R5, R6;
	vec4 mask1;

	// Calculate megatexture masks using maths
	R1 =	abs(texcoord3);
	R2 =	abs(texcoord4);

	// megaMask parameters packing in the vertex shader saves one pixel shader instruction	
	R1 =	max(R1, R2);
	
	mask1 =	clamp((-32.0*R1)+16.0, 0.0, 1.0);
	
	// load the minimum quality image first, which will be used
	// if none of the other textures have a valid mask
	vec2 scaled = texcoord7.xy;
	
	$if ( r_shaderQuality < 1 )
		
		if (mask1.w==1.0)
		{
			R5.xy = scaled * megatextureparams_4.xy;
			combined = texture2D(megatexturelevel_4, R5.xy);
			//combined = vec4(0, 1, 0, 1);
		}
		else
		 
	$endif
	
	if (mask1.x==0.0)
	{
		R1.xy = scaled * megatextureparams_0.xy;
		combined = texture2D(megatexturelevel_0, R1.xy);
	}
	else
	{
		// load the multi-resolution textures and sum up with the masks
		
		if ( mask1.y < 1.0) {
			if (mask1.x < 1.0) {
				R1.xy = scaled * $megaTextureParams_0.xy;
				R1 = texture2D($megaTextureLevel_0, R1.xy);	// lowest quality moving image
				R2.xy = scaled * $megaTextureParams_1.xy;
				R2 = texture2D($megaTextureLevel_1, R2.xy);
				combined = R1;
				combined = mix(combined, R2, mask1.x);
			}
			else {
				R2.xy = scaled * $megaTextureParams_1.xy;
				R2 = texture2D($megaTextureLevel_1, R2.xy);
				combined = R2;
				if ( mask1.y > 0.0) {
					R3.xy = scaled * $megaTextureParams_2.xy;
					R3 = texture2D($megaTextureLevel_2, R3.xy);
					combined = mix(combined, R3, mask1.y);
				}
			}
		}
		else {
			if ( mask1.z < 1.0) {
				R3.xy = scaled * $megaTextureParams_2.xy;
				R3 = texture2D($megaTextureLevel_2, R3.xy);
				combined = R3;
				if ( mask1.z > 0.0) {
					R4.xy = scaled * $megaTextureParams_3.xy;
					R4 = texture2D($megaTextureLevel_3, R4.xy);
				
					combined = mix(combined, R4, mask1.z);
				}
			}
			else {
				R4.xy = scaled * $megaTextureParams_3.xy;
				R4 = texture2D($megaTextureLevel_3, R4.xy);
				combined = R4;
				if ( mask1.w > 0.0) {
					R5.xy = scaled * $megaTextureParams_4.xy;
					R5 = texture2D($megaTextureLevel_4, R5.xy);
				
					combined = mix(combined, R5, mask1.w);
				}
			}
		}
		
	}	
	
	// Modulate with diffuse color
	combined.xyz = combined.xyz * $diffuseColor.xyz;
	
	