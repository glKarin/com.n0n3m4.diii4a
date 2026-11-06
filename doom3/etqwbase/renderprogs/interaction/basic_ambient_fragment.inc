
varying vec4 var_TexCoord4; // var_TexDiffuse
varying vec4 var_TexCoord0; // var_V
varying vec4 var_TexCoord1; // var_TexNormal
varying vec4 var_TexCoord6; // var_TangentToWorldMatrix[1]
varying vec4 var_Color;
varying vec4 var_TexCoord5; // var_TangentToWorldMatrix[0]
varying vec4 var_TexCoord7; // var_TangentToWorldMatrix[2]
varying vec4 var_TexCoord2;
$if ( r_megaDrawMethod != 0 )
varying vec4 var_Color2;
$endif

void main (void) {
    // ARB_precision_hint_fastest;
    vec4 R0;
    vec4 R1;
    vec4 color;
    vec4 localNormal;
    vec4 worldNormal;
    vec4 ambient;
    vec4 localAmbient;
    vec4 specLook;
    vec4 diffuse;
    vec4 subOne = vec4(-1.0, -1.0, -1.0, -1.0);
    vec4 scaleTwo = vec4(2.0, 2.0, 2.0, 2.0);
$ifdef parallax
    vec4 height;
    vec4 viewofs;
    vec4 _diffTC;
    vec4 _bumpTC;
    height.x = texture2D ($heightMap, var_TexCoord4.xy).x;
    // TEX  height.x, fragment.texcoord[4],  $diffuseMap, 2D;
    height.x = height.x * -0.0015;
    viewofs.xyz = var_TexCoord0.xyz * var_TexCoord0.xyz;
    viewofs.w = 1.0 / sqrt (viewofs.w);
    viewofs.xyz = viewofs.w * var_TexCoord0.xyz;
    _diffTC = viewofs * height.x + var_TexCoord4;
    _bumpTC = viewofs * height.x + var_TexCoord1;
$else
    vec4 _diffTC = var_TexCoord4;
    vec4 _bumpTC = var_TexCoord1;
$endif
$ifdef enable_detail
    vec4 DTLCOORD;
    DTLCOORD.xy = _bumpTC * detailMult + detailMult.zw;
$endif
$ifdef enable_instdither
    R1.xy = gl_FragCoord.xy * 0.0625;
    R1 = texture2D ($noiseMap, R1.xy);
    R1.w = var_TexCoord6.w < R1.w ? 1.0 : 0.0;
    if (-R1.w < 0.0) discard;
$endif
    // coverage dither mask
$if/*def*/ r_useDitherMask
    R1.xy = gl_FragCoord.xy * 0.0625;
    R1 = texture2D ($noiseMap, R1.xy);
    R1.w = $coverage.x < R1.w ? 1.0 : 0.0;
    if (-R1.w < 0.0) discard;
    // MUL  R1.xy, fragment.position, 0.015625;
    // MUL R1.xy, fragment.position, 0.0625;
    // TEX  R0.r, R1,  $noiseMap, 2D;
    // SUB  R0.a, 0.5, R0.r;
    // KIL  R0.a;
$endif
    // modulate by the diffuse map and constant diffuse factor
    diffuse = texture2D ($diffuseMap, _diffTC.xy);
$ifdef alphatest_kill
    diffuse.a = diffuse.a - $alphaThresh.x;
    if (diffuse.a < 0.0) discard;
$endif
$ifdef alphatest_alphatocoverage
    R0.a = diffuse.a - 0.1;
    if (R0.a < 0.0) discard;
$endif
$ifdef enable_detailweight_const
    vec4 weight;
    weight = 1.0;
$endif
$ifdef enable_detailweight_vtxalpha
    vec4 weight;
    weight = var_Color.a;
$endif
$ifdef enable_detailweight_diffalpha
    vec4 weight;
    weight = vec4(diffuse.a);
$endif
$ifdef enable_detailweight_tex
    vec4 weight;
    weight = texture2D ($detailWeightMap, _diffTC.xy);
$endif
$ifdef enable_detail
    weight = clamp (weight * var_TexCoord1.w, 0.0, 1.0);
$endif
    localNormal = texture2D ($bumpMap, _bumpTC.xy);
$if !r_dxnNormalMaps
    localNormal.x = localNormal.a;
$endif
    localNormal.xyz = (localNormal * scaleTwo + subOne).xyz;
$ifdef enable_bumpdetail
    vec4 localDetailNormal;
    localDetailNormal = texture2D ($bumpDetailMap, DTLCOORD.xy);
    localDetailNormal.x = localDetailNormal.a;
    localDetailNormal.xy = (localDetailNormal * scaleTwo + subOne).xy;
    localNormal.xy = weight.x * localDetailNormal + localNormal;
$endif
    // Renormalize
$if r_normalizeNormalMaps
    localNormal.z = 0.0;
    R1.x = dot (localNormal .xyz, localNormal .xyz);
    R1.x = 1.0 + -R1.x;
    R1.x = 1.0 / sqrt (R1.x);
    localNormal.z = 1.0 / R1.x;
$endif
    // put normal into world space
    worldNormal.x = dot (var_TexCoord5 .xyz, localNormal .xyz);
    worldNormal.y = dot (var_TexCoord6 .xyz, localNormal .xyz);
    worldNormal.z = dot (var_TexCoord7 .xyz, localNormal .xyz);
    // get diffuse lighting from cubemap
    ambient = textureCube ($ambientCubeMap, worldNormal.xyz);
    ambient = ambient * $ambientBrightness.x;
$ifdef enable_diffdetail
    vec4 detailDiff;
    detailDiff = texture2D ($diffuseDetailMap, DTLCOORD.xy);
    detailDiff = detailDiff + -0.5;
    diffuse = weight.x * detailDiff + diffuse;
$endif
    color = diffuse * $diffuseColor;
    // kill if we need this
    // modulate by ambient
    color = ambient * color;
$if !r_shaderSkipSpecCubeMaps
    // calc reflection vector: i - 2 * dot(i, n) * n
    R1 = vec4( dot (var_TexCoord2 .xyz, worldNormal .xyz) );
    R1 = R1 + R1;
    R0 = -worldNormal * R1.x + var_TexCoord2;
    // DP3  ambient.a, fragment.texcoord[2], fragment.texcoord[2];
    // RSQ  ambient.a, ambient.a;
    // MUL  ambient, fragment.texcoord[2], ambient.a;
    // MAD  ambient, ambient, 0.5, 0.5;
    // get specular from cubemap
    ambient = textureCube ($specularCubeMap, R0.xyz);
    // modulate by the specular map * 2
    specLook = texture2D ($specularMap, _diffTC.xy);
  $ifdef enable_specdetail
    vec4 dtlspecLook;
    dtlspecLook = texture2D ($specDetailMap, DTLCOORD.xy);
    dtlspecLook = dtlspecLook + -0.5;
    specLook = clamp (weight.x * dtlspecLook + specLook, 0.0, 1.0);
  $endif
    R0 = specLook + specLook;
    color = R0 * ambient + color;
$endif
    // multiply with ambient brightness
    color = $ambientScale.x * color;
    // do some special effects if we're compiling certain shaders
    // we could move this to a separate file but this saves us from
    // duplicating the shader
$ifdef self_illum
    // Simple self illumination
    R0 = texture2D ($selfIllumMap, _diffTC.xy);
  $ifdef self_illum_glow
    R0.rgb = clamp (R0.a * parameters2 + R0, 0.0, 1.0);
  $endif
  $ifdef self_illum_modulate
    color = parameters * R0 + color;
  $else
    color = color + R0;
  $endif
$endif
$ifdef strogg_effects
    // "Beetle shell" empirical shader
    R1 = vec4( dot (var_TexCoord2 .xyz, worldNormal .xyz) );
    R0 = texture2D ($stroggShellMap, R1.xy);
    R1 = specLook * 1.8 + 0.05;
    R0 = R1 * R0;
    R0 = sunColor * R0;
    // so it goes dimmer at night
    color = color + R0;
$endif
    // modify by the vertex color
$if (r_megaDrawMethod != 0)
    color.rgb = mix (color, $fogColor, var_Color2.x).xyz;
$endif
    gl_FragColor = color * var_Color;
$ifdef alphatest_alphatocoverage
    gl_FragColor.a = diffuse.a * 1.5;
$else
    gl_FragColor.a = $ambientScale.y;
$endif
$ifdef enable_detail
    // MOV  result.color.rgb, fragment.texcoord[7].w;
$endif
    // MAD  result.color.rgb, worldNormal, 0.5, 0.5;
}
