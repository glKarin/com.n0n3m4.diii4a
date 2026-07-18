

$define fog_depths $fogDepths
$if ( r_megaDrawMethod == 0 )
$define fog_parameters $fogParams
$define fog_eyePos $viewOriginWorld.xyz
$endif
$define fog_viewOrigin $viewOrigin.xyz
$if ( r_megaDrawMethod == 0 )
$define fog_matrix_x $transposedModelMatrix_x
$define fog_matrix_y $transposedModelMatrix_y
$define fog_matrix_z $transposedModelMatrix_z
$endif
//MAD		result.color.secondary, _F1.x, $fogDepths.z, $fogDepths.w; 

vec2 Extinction(vec3 inpos) {
$if ( r_megaDrawMethod != 0 )
	vec3 diff = inpos - fog_viewOrigin;
	float d = length( diff );
	return 1.0-clamp( d * fog_depths.z + fog_depths.w, 0.0, 1.0 );
$else
	vec3 eye = fog_eyePos;

	vec4 iP = vec4(inpos,1.0);
	vec3 pos;
	pos.x = dot( iP, fog_matrix_x );
	pos.y = dot( iP, fog_matrix_y );
	pos.z = dot( iP, fog_matrix_z );

	float oneOverA = fog_parameters.x;
	float oneOverB = fog_parameters.y;
	float C = fog_parameters.z;

	//Cheap clutch for horizontal rays
	if (abs(eye.z-pos.z) < 100.0) {
		eye.z = pos.z + 100.0;
	}

	vec3 ray = (pos - eye);

	//Deviation from the vertical ray
	vec3 rayn = normalize(ray);
	vec3 up = vec3(0.0,0.0,1.0);
	float costheta = dot(up, rayn);

	float z1 = max(pos.z+C,0.0);
	float z2 = max(eye.z+C,0.0);

	//Integrate along a vertical ray
	float dens = pow(0.5,z2*oneOverB)-pow(0.5,z1*oneOverB);

	//Correct for horizontal rays
	dens = dens/costheta;

	//Multiply with distance
	float thick = dens*(length(ray)*oneOverA);
	float T = pow(0.5, thick);

	return vec2(T, rayn.z);
$endif
}
