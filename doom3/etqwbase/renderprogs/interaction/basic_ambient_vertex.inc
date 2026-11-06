
$ifdef instance
attribute vec4 attr_TexCoord5;
attribute vec4 attr_TexCoord6;
attribute vec4 attr_TexCoord7;
attribute vec4 attr_TexCoord2;
$endif

varying vec4 var_TexCoord6; // var_TangentToWorldMatrix[1]
varying vec4 oPos;
varying vec4 var_TexCoord1; // var_TexNormal
varying vec4 var_TexCoord4; // var_TexDiffuse
varying vec4 var_Color;
varying vec4 var_TexCoord5; // var_TangentToWorldMatrix[0]
varying vec4 var_TexCoord7; // var_TangentToWorldMatrix[2]
varying vec4 var_TexCoord2;
varying vec4 var_TexCoord0; // var_V
$if ( r_megaDrawMethod != 0 )
varying vec4 var_Color2;
$endif

void main ( void ) {
    gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
$ifndef instance
  $ifndef deform
	// ARB_position_invariant;
  $endif
$endif
	vec4 bitangent = vec4(attr_Bitangent, 1.0);
	vec4 R0;
	vec4 R1;
	vec4 R2;
$ifdef instance
	vec4 _pos;
	vec4 _nrm;
	vec4 _tan;
	_pos.x = dot ( attr_TexCoord5 , $positionAttrib );
	_pos.y = dot ( attr_TexCoord6 , $positionAttrib );
	_pos.z = dot ( attr_TexCoord7 , $positionAttrib );
	_pos.w = $positionAttrib.w;
	_nrm.x = dot ( attr_TexCoord5 .xyz , $normalAttrib .xyz );
	_nrm.y = dot ( attr_TexCoord6 .xyz , $normalAttrib .xyz );
	_nrm.z = dot ( attr_TexCoord7 .xyz , $normalAttrib .xyz );
	_tan.x = dot ( attr_TexCoord5 .xyz , $tangentAttrib .xyz );
	_tan.y = dot ( attr_TexCoord6 .xyz , $tangentAttrib .xyz );
	_tan.z = dot ( attr_TexCoord7 .xyz , $tangentAttrib .xyz );
	var_TexCoord6.w = attr_TexCoord2;
	mat4 mvp = u_modelViewProjectionMatrix;
	oPos = gl_Position;
	// Transform the vertex to clip coordinates.
	oPos.x = dot ( mvp[0], _pos );
	oPos.y = dot ( mvp[1], _pos );
	oPos.z = dot ( mvp[2], _pos );
	oPos.w = dot ( mvp[3], _pos );
$else
  $ifdef deform
	vec4 _pos;
	mat4 mvp = u_modelViewProjectionMatrix;
	oPos = gl_Position;
	// Transform the vertex to clip coordinates.
	R0.x = parameters.x * VERTEX_BYTE_COLOR( $colorAttrib.w );
	_pos.xyz = R0.x * $normalAttrib + $positionAttrib;
	_pos.w = $positionAttrib.w;
	oPos.x = dot ( mvp[0], _pos );
	oPos.y = dot ( mvp[1], _pos );
	oPos.z = dot ( mvp[2], _pos );
	oPos.w = dot ( mvp[3], _pos );
  $else
	vec4 _pos = $positionAttrib;
  $endif
  $if !r_32ByteVtx
	vec4 _nrm = vec4($normalAttrib, 1.0);
	vec4 _tan = vec4($tangentAttrib, 1.0);
  $else
	vec4 _nrm;
	vec4 _tan;
	_nrm = vec4($normalAttrib, 1.0);
	_tan = vec4($tangentAttrib, 1.0);
  $endif
$endif
	vec4 defaultTexCoord = vec4(0.0,0.5,0.0,1.0);
	// textures 1 takes the base coordinates by the texture matrix
	var_TexCoord1 = defaultTexCoord;
	var_TexCoord1.x = dot ( $texCoordAttrib , $bumpMatrix_s );
	var_TexCoord1.y = dot ( $texCoordAttrib , $bumpMatrix_t );
	// textures 4 takes the base coordinates by the texture matrix
	var_TexCoord4 = defaultTexCoord;
	var_TexCoord4.x = dot ( $texCoordAttrib , $diffuseMatrix_s );
	var_TexCoord4.y = dot ( $texCoordAttrib , $diffuseMatrix_t );
	// generate the vertex color, which can be 1.0, color, or 1.0 - color
	var_Color = $colorAttrib * $colorModulate + $colorAdd;
	// build tangent space -> world space conversion matrix
	var_TexCoord5.x = dot ( transposedModelMatrix_x .xyz , _tan .xyz );
	var_TexCoord6.x = dot ( transposedModelMatrix_y .xyz , _tan .xyz );
	var_TexCoord7.x = dot ( transposedModelMatrix_z .xyz , _tan .xyz );
	var_TexCoord5.y = dot ( transposedModelMatrix_x .xyz , bitangent .xyz );
	var_TexCoord6.y = dot ( transposedModelMatrix_y .xyz , bitangent .xyz );
	var_TexCoord7.y = dot ( transposedModelMatrix_z .xyz , bitangent .xyz );
	var_TexCoord5.z = dot ( transposedModelMatrix_x .xyz , _nrm .xyz );
	var_TexCoord6.z = dot ( transposedModelMatrix_y .xyz , _nrm .xyz );
	var_TexCoord7.z = dot ( transposedModelMatrix_z .xyz , _nrm .xyz );
	// calculate vector to eye
	R0.x = dot ( $transposedModelMatrix_x , _pos );
	R0.y = dot ( $transposedModelMatrix_y , _pos );
	R0.z = dot ( $transposedModelMatrix_z , _pos );
	// normalize
	vec4 viewtovec;
	viewtovec.xyz = (R0 - $viewOriginWorld).xyz;
	viewtovec.w = dot ( viewtovec .xyz , viewtovec .xyz );
	R0.w = 1.0 / sqrt ( viewtovec.w );
	var_TexCoord2 = viewtovec * -R0.w;
	// MOV  result.texcoord[2], R0;
$ifdef  parallax
	// calculate vector to light in R0
	R0 = $viewOrigin - _pos;
	// put into texture space for TEX6
	var_TexCoord0.x = dot ( _tan .xyz , R0 .xyz );
	var_TexCoord0.y = dot ( bitangent .xyz , R0 .xyz );
	var_TexCoord0.z = dot ( _nrm .xyz , R0 .xyz );
$endif
$ifdef enable_detail_fade
	mat4 mv = u_modelViewMatrix;
	R0.w = dot ( -mv[2], _pos );
	R0.w = R0.w - detailFade.x;
	var_TexCoord1.w = R0.w * detailFade.y + 1.0;
$endif
$if ( r_megaDrawMethod != 0 )
	R2 = _pos - $viewOrigin;
	R1.x = dot ( R2 .xyz , R2 .xyz );
	R1.y = 1.0 / sqrt ( R1.x );
	R1.x = R1.y * R1.x;
	var_Color2 = vec4( R1.x * $fogDepths.z + $fogDepths.w );
$endif
}
