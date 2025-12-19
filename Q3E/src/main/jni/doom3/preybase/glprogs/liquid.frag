#version 100

//#pragma optimize(off)

precision mediump float;

// per-pixel reflection-refraction liquid
varying vec4 var_TexCoord0; // texCoord[0] is the normal map texcoord
varying highp vec4 var_TexCoord1; // texCoord[1] is the vector to the eye in global space //karin: must high precison on GLSL
varying vec4 var_TexCoord2; // texCoord[2] is the surface tangent to global coordiantes
varying vec4 var_TexCoord3; // texCoord[3] is the surface bitangent to global coordiantes
varying vec4 var_TexCoord4; // texCoord[4] is the surface normal to global coordiantes
varying vec4 var_TexCoord6; // texCoord[6] is the copied deform magnitude
//varying vec4 var_TexCoord7; // texCoord[7] is the copied scroll

uniform highp vec4 u_nonPowerOfTwo; // env[0] is the 1.0 to _currentRender conversion
uniform highp vec4 u_windowCoords; // env[1] is the fragment.position to 0.0 - 1.0 conversion
uniform vec4 u_fragmentParm0; // scroll
uniform vec4 u_fragmentParm1; // deform magnitude (1.0 is reasonable, 2.0 is twice as wavy, 0.5 is half as wavy, etc)
uniform vec4 u_fragmentParm2; // refract color
uniform vec4 u_fragmentParm3; // reflect color

uniform samplerCube u_fragmentCubeMap0; // texture 0 is the environment cube map
uniform sampler2D u_fragmentMap1; // texture 1 is the normal map
uniform sampler2D u_fragmentMap2; // texture 2 is _currentRender

void main(void)
{
    vec4 scroll = u_fragmentParm3;

    const vec4 subOne = vec4( -1.0, -1.0, -1.0, -1.0 );
    const vec4 scaleTwo = vec4( 2.0, 2.0, 2.0, 2.0 );

    // load the filtered normal map, then normalize to full scale,
    vec4 R0 = 0.02 * scroll + var_TexCoord0;
    R0 = 0.9 * R0;
    vec4 localNormal = texture2D(u_fragmentMap1, R0.st);

    R0 = 0.1 * scroll.zyxy + var_TexCoord0;
    R0 = 0.125 * R0;
    R0 = texture2D(u_fragmentMap1, R0.st);
    localNormal = R0 + localNormal;

    R0 = -0.17 * scroll.wxxy + var_TexCoord0;
    R0 = 0.5 * R0;
    R0 = texture2D(u_fragmentMap1, R0.st);
    localNormal = R0 + localNormal;

    R0 = 0.2 * scroll.wwxy + var_TexCoord0;
    R0 = 0.25 * R0;
    R0 = texture2D(u_fragmentMap1, R0.st);
    localNormal = R0 + localNormal;

    localNormal = localNormal.agbr * scaleTwo - 4.0;
    localNormal.xy = localNormal.xy * 1.5;

    // transform the surface normal by the local tangent space
    vec4 globalNormal;
    globalNormal.x = dot(localNormal.xyz, var_TexCoord2.xyz);
    globalNormal.y = dot(localNormal.xyz, var_TexCoord3.xyz);
    globalNormal.z = dot(localNormal.xyz, var_TexCoord4.xyz);
    globalNormal.w = 1.0;

    // normalize
    vec4 R1;
    R1.x = dot(globalNormal.xyz, globalNormal.xyz);
    R1.x = 1.0 / sqrt(R1.x);
    globalNormal.xyz = (globalNormal * R1.x).xyz;

    // normalize vector to eye
    highp float R0_x = dot(var_TexCoord1.xyz, var_TexCoord1.xyz); //karin: must high precison on GLSL
    R0_x = 1.0 / sqrt(R0_x);
    vec4 globalEye = var_TexCoord1 * R0_x;

    // calculate reflection vector
    R1 = vec4(dot(globalEye.xyz, globalNormal.xyz));
    R0 = R1 * globalNormal;
    R0 = R0 * scaleTwo - globalEye;

    // read the environment map with the reflection vector
    R0 = textureCube(u_fragmentCubeMap0, R0.xyz);

    // calculate fresnel
    vec4 fresnel = vec4(1.0 - R1); // 1 - V dot N
    fresnel = vec4(pow(fresnel.x, u_fragmentParm0.x)); // assume index is 1
    fresnel = fresnel * u_fragmentParm0.y + u_fragmentParm0.z;

    // calculate the screen texcoord in the 0.0 to 1.0 range
    R1 = gl_FragCoord * u_windowCoords;

    // offset by the scaled globalNormal and clamp it to 0.0 - 1.0
    R1 = clamp(globalNormal * var_TexCoord6 + R1, 0.0, 1.0);

    // scale by the screen non-power-of-two-adjust
    R1 = R1 * u_nonPowerOfTwo;

    // load the screen render
    R1 = texture2D(u_fragmentMap2, R1.st);

    R1 = R1 * u_fragmentParm1;
    R0 = R0 * u_fragmentParm2;

    vec3 color = mix(R1, R0, fresnel).xyz; // lerp refract and reflect

    gl_FragColor = vec4(color, 1.0);
}
