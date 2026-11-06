
/*
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

		uniform vec4 $lightProject_s;
		uniform vec4 $lightProject_t;
		uniform vec4 $lightProject_q;

		uniform vec4 $lightFalloff_s;

		uniform vec4 $lightOrigin;
*/
		
		varying	vec4 texcoord3;
		varying	vec4 texcoord4;
		varying	vec4 texcoord6;
		varying	vec4 texcoord7;
		varying	vec4 vertexColor;

		$ifdef r_useShadowMapping
		$include "megatexture/interaction_glsl_vertex_shadowmapping.inc"
		$endif
		

		void main()
		{
			gl_Position = u_modelViewProjectionMatrix * attr_Vertex;

			// Pack for best efficiency (reduces number of fragment shader instructions)
			// This could ideally be done before sending the constants (would eliminate the need for the 3 lines below)
			vec4 inc0 = vec4($megaMaskParams_1.x, $megaMaskParams_2.x, $megaMaskParams_3.x, $megaMaskParams_4.x);
			vec4 inc1 = vec4($megaMaskParams_1.y, $megaMaskParams_2.y, $megaMaskParams_3.y, $megaMaskParams_4.y);
			vec4 mul  = vec4($megaMaskParams_1.w, $megaMaskParams_2.w, $megaMaskParams_3.w, $megaMaskParams_4.w);

			texcoord3 = 0.5 - (($texCoordAttrib.x * mul) + inc0);
			texcoord4 = 0.5 - (($texCoordAttrib.y * mul) + inc1);

			// images just have a scale
			texcoord7 = $texCoordAttrib;
			
			$if r_detailTexture
				texcoord7.zw = ($texCoordAttrib.xy - 0.5) * $megaDetailTextureParams.x;
			$endif
			
			texcoord6.x = dot(attr_Vertex, $lightProject_s);
			texcoord6.y = dot(attr_Vertex, $lightProject_t);
			texcoord6.z = dot(attr_Vertex, $lightFalloff_s);
			texcoord6.w = dot(attr_Vertex, $lightProject_q);

			// calculate vector to light in R0 (object space)
			float LightIntensity = max(0.0, dot($normalAttrib, normalize($lightOrigin.xyz - attr_Vertex.xyz)));
			
			vertexColor = vec4(LightIntensity, LightIntensity, LightIntensity, 1.0);

			$ifdef r_useShadowMapping
			$include "megatexture/interaction_glsl_vertex_shadowmapping.inc"
			$endif
		}
