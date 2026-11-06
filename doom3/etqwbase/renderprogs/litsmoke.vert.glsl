/**

	Template glsl vertex program

*/

varying vec4 outdata_col;
varying vec4 outdata_atten;
varying vec2 outdata_tex;
varying vec3 outdata_tang;
varying vec3 outdata_bino;
varying vec3 outdata_norm;
varying vec3 outdata_toLight0;
varying vec3 outdata_toLight1;
varying vec3 outdata_toLight2;
varying vec3 outdata_toLight3;

$define lightOrigin0 $lightOrigin_0
$define lightOrigin1 $lightOrigin_1
$define lightOrigin2 $lightOrigin_2
$define lightOrigin3 $lightOrigin_3
$define lightFallOff0 $lightFalloff_0
$define lightFallOff1 $lightFalloff_1
$define lightFallOff2 $lightFalloff_2
$define lightFallOff3 $lightFalloff_3

float GetZ( vec2 value, float sign ) {
	return ( sqrt( 1.0f - ( dot( value.xy, value.xy ) ) ) * ( sign - 1.0 ) );
}


#define TO_LIGHT( lightOrigin, lightFallOff, dest, atten )	\
toLight = lightOrigin.xyz - indata_center;\
temp = toLight * lightFallOff.xyz;\
atten = length( temp );\
toLight = normalize( toLight );\
dest.x = dot( tang, toLight );\
dest.y = dot( bino, toLight );\
dest.z = dot( normal, toLight )

$include "atmosphere.glsl"

$define Matrix_x $transposedModelMatrix_x
$define Matrix_y $transposedModelMatrix_y
$define Matrix_z $transposedModelMatrix_z

void main() {
	vec4 indata_tex = attr_TexCoord;
	vec3 indata_center = attr_Normal;
	vec3 indata_tang = attr_Tangent;
	vec4 indata_col = VERTEX_BYTE_COLOR($colorAttrib);

	$if r_32ByteVtx
	indata_tex.xy *= 1.0/4096.0;
#ifdef TRAIL
	indata_center.xy *= 1.0 / 32767.0;
	indata_tang.xy *= 1.0 / 32760.0;
	indata_center.z = GetZ( indata_center.xy, indata_signs.x );
	indata_tang.z = GetZ( indata_tang.xy, indata_signs.y );
	indata_tang.w = ( indata_signs.z - 1.0 );
#else
	indata_center.z = indata_tang.x;
	indata_tang = indata_signs;
#endif
	$endif

	outdata_tex.x = dot( indata_tex, $diffuseMatrix_s );
	outdata_tex.y = dot( indata_tex, $diffuseMatrix_t );
	outdata_col = indata_col;

	//
	//	Atmosphere
	//
#if 0
	vec2 r = vec2(0.5, 1.0 );
#else

	vec2 r = Extinction(indata_center);
#endif

	vec4 tempc = vec4( indata_col.rgb, r.x * indata_col.a );
	outdata_col= tempc;

#ifdef TRAIL
	vec3 normal = indata_center;
	vec3 bino = indata_tang.xyz;
	vec3 tang = cross( bino, normal );

	vec3 toView = normalize( $viewOrigin.xyz - indata_center );
	outdata_col.a *= 1.0-pow( abs( dot( toView, tang ) ), 32.0 );
#else
	// Calculate the tangent space for this billboard, this is a bit tricky as our billboards are all lying in planes paralel
	// to the near plane, this makes for cheap billboard calculations on the CPU.  But then they all lit identical because they
	// all have identical tangent space, and it is also dependent on the view angles. So we want a view-vector billboard tangent
	// space. We fake this by taking the view-vector and thus not the billboard-normal as the normal for our tangent space and
	// rederive the other two vectors.
	vec3 normal = normalize( $viewOrigin.xyz - indata_center );
	vec3 bino = cross( normal, indata_tang.xyz );
	vec3 tang = cross( normal, bino );
#endif

#if 1
	// Tangent -> World (for the ambient cubemap lookup)
	outdata_tang.x = dot( tang, Matrix_x.xyz );
	outdata_tang.y = dot( bino, Matrix_x.xyz );
	outdata_tang.z = dot( normal, Matrix_x.xyz );

	outdata_bino.x = dot( tang, Matrix_y.xyz );
	outdata_bino.y = dot( bino, Matrix_y.xyz );
	outdata_bino.z = dot( normal, Matrix_y.xyz );

	outdata_norm.x = dot( tang, Matrix_z.xyz );
	outdata_norm.y = dot( bino, Matrix_z.xyz );
	outdata_norm.z = dot( normal, Matrix_z.xyz );
#else
	outdata_tang = tang;
	outdata_bino = bino;
	outdata_norm = normal;
#endif

	// toLight in tangent space
	vec3 toLight;
	vec3 temp;
	float len;

	outdata_atten = vec4(0.0);
#ifdef LIGHT_1
	TO_LIGHT( lightOrigin0, lightFallOff0, outdata_toLight0, outdata_atten.r );
#endif
#ifdef LIGHT_2
	TO_LIGHT( lightOrigin1, lightFallOff1, outdata_toLight1, outdata_atten.g );
#endif
#ifdef LIGHT_3
	TO_LIGHT( lightOrigin2, lightFallOff2, outdata_toLight2, outdata_atten.b );
#endif
#ifdef LIGHT_4
	TO_LIGHT( lightOrigin3, lightFallOff3, outdata_toLight3, outdata_atten.a );
#endif

#ifdef LIGHT_1
	// Atten just contains the lengt of the scaled tolight vector now make it in the 0-1 range
	outdata_atten = 1.0 - min( outdata_atten, 1.0 );
#endif

	//outdata_atten.y = 1.0f;
	gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}
