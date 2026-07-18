/**

	Template glsl fragment program

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

$define lightColor0 $lightColor_0
$define lightColor1 $lightColor_1
$define lightColor2 $lightColor_2
$define lightColor3 $lightColor_3

#ifdef TRAIL
$define normalScroll $parameters
#endif

$define normalMap $bumpMap
$define ambientMap $ambientCubeMapSun

$include "atmosphere.glsl"

vec3 CalculateLight( vec3 normal, vec3 direction, vec3 color, float falloff ) {
	return max( dot( normal, direction), 0.0 ) * color * falloff;
}

void main() {
	vec4 diffuse = texture2D( $diffuseMap, outdata_tex );
	if ( diffuse.a < 0.01 ) discard;

#ifdef TRAIL
	vec4 normalt = texture2D( normalMap, outdata_tex - normalScroll.xy ) * 2.0 - 1.0;
	//vec4 normalt = ( 0.5 * texture2D( normalMap, outdata_tex ) + 0.5 * texture2D( normalMap, outdata_tex - normalScroll ) ) * 2.0 - 1.0;
#else
	vec4 normalt = texture2D( normalMap, outdata_tex ) * 2.0 - 1.0;
#endif
	$if !r_dxnNormalMaps
	normalt.x = normalt.a;
	$endif
	normalt.z = sqrt( 1.0 - dot( normalt.xy, normalt.xy ) );
	vec3 normal = normalt.rgb;
	diffuse *= outdata_col;

	vec3 wNormal;
	wNormal.x = dot( outdata_tang, normal );
	wNormal.y = dot( outdata_bino, normal );
	wNormal.z = dot( outdata_norm, normal );

	vec3 light = textureCube( ambientMap, wNormal ).rgb * 4.0;
#ifdef LIGHT_1
	light += CalculateLight( normal, normalize( outdata_toLight0 ), lightColor0.rgb, outdata_atten.x );
#endif
#ifdef LIGHT_2
	light += CalculateLight( normal, normalize( outdata_toLight1 ), lightColor1.rgb, outdata_atten.y );
#endif
#ifdef LIGHT_3
	light += CalculateLight( normal, normalize( outdata_toLight2 ), lightColor2.rgb, outdata_atten.z );
#endif
#ifdef LIGHT_4
	light += CalculateLight( normal, normalize( outdata_toLight3 ), lightColor3.rgb, outdata_atten.w );
#endif

    gl_FragColor = vec4( light * diffuse.rgb, diffuse.a );
    //return vec4( diffuse.rgb, diffuse.a );
}
	

