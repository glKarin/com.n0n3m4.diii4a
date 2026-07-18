		attribute vec4 $positionAttrib;
		attribute vec3 $normalAttrib;
		attribute vec4 $texCoordAttrib;
		
		uniform vec4 $megaMaskParams_0;
		uniform vec4 $megaMaskParams_1;
		uniform vec4 $megaMaskParams_2;
		uniform vec4 $megaMaskParams_3;
		uniform vec4 $megaMaskParams_4;
		uniform vec4 $megaMaskParams_5;
		uniform vec4 $megaDetailTextureParams;

		varying	vec4 texcoord3;
		varying	vec4 texcoord4;
		varying	vec3 texcoord6;
		varying	vec4 texcoord7;
		
		void main()
		{
		
			$if r_useARBPositionInvariant
			gl_Position = ftransform();
			$else
			gl_Position.x = dot( vec4(gl_ModelViewProjectionMatrix[0].x, gl_ModelViewProjectionMatrix[1].x, gl_ModelViewProjectionMatrix[2].x, gl_ModelViewProjectionMatrix[3].x ) , gl_Vertex );
			gl_Position.y = dot( vec4(gl_ModelViewProjectionMatrix[0].y, gl_ModelViewProjectionMatrix[1].y, gl_ModelViewProjectionMatrix[2].y, gl_ModelViewProjectionMatrix[3].y ) , gl_Vertex );
			gl_Position.z = dot( vec4(gl_ModelViewProjectionMatrix[0].z, gl_ModelViewProjectionMatrix[1].z, gl_ModelViewProjectionMatrix[2].z, gl_ModelViewProjectionMatrix[3].z ) , gl_Vertex );
			gl_Position.w = dot( vec4(gl_ModelViewProjectionMatrix[0].w, gl_ModelViewProjectionMatrix[1].w, gl_ModelViewProjectionMatrix[2].w, gl_ModelViewProjectionMatrix[3].w ) , gl_Vertex );
			$endif
			
			// Pack for best efficiency (reduces number of fragment shader instructions)
			// This could ideally be done before sending the constants (would eliminate the need for the 3 lines below)
			vec4 inc0 = vec4($megaMaskParams_1.x, $megaMaskParams_2.x, $megaMaskParams_3.x, $megaMaskParams_4.x);
			vec4 inc1 = vec4($megaMaskParams_1.y, $megaMaskParams_2.y, $megaMaskParams_3.y, $megaMaskParams_4.y);
			vec4 mul  = vec4($megaMaskParams_1.w, $megaMaskParams_2.w, $megaMaskParams_3.w, $megaMaskParams_4.w);

			texcoord3 = 0.5 - (($texCoordAttrib.x * mul) + inc0);
			texcoord4 = 0.5 - (($texCoordAttrib.y * mul) + inc1);
			
			texcoord6 = $normalAttrib;
			
			// images just have a scale
			texcoord7 = $texCoordAttrib;
			
			$if r_detailTexture
				texcoord7.zw = ($texCoordAttrib.xy - 0.5) * $megaDetailTextureParams.x;
			$endif
		}