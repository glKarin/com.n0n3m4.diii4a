#version 100
//#pragma optimize(off)

precision mediump float;

varying vec2 var_TexCoord;
varying lowp vec4 var_Color;
varying mat3 var_TangentToWorldMatrix;
varying highp vec3 var_toEyeWorld; //karin: must highp in GLES

uniform samplerCube u_fragmentCubeMap0;
uniform sampler2D u_fragmentMap1; // normal map
uniform lowp vec4 u_glColor;

void main(void)
{
    // per-pixel cubic reflextion map calculation

    // texture 0 is the environment cube map
    // texture 1 is the normal map

    // load the filtered normal map, then normalize to full scale,
    vec3 localNormal = texture2D(u_fragmentMap1, var_TexCoord).agb; // doom3's normal map is not rgb
    localNormal = localNormal * vec3(2.0) - vec3(1.0);
    localNormal.z = sqrt(max(0.0, 1.0 - localNormal.x * localNormal.x - localNormal.y * localNormal.y));
    localNormal = normalize(localNormal);

    // transform the surface normal by the local tangent space
    vec3 globalNormal = var_TangentToWorldMatrix * localNormal;

    // normalize vector to eye
    vec3 globalEye = normalize(var_toEyeWorld);

    // calculate reflection vector
    float dotEN = dot(globalEye, globalNormal);
    vec3 globalReflect = 2.0 * (dotEN * globalNormal) - globalEye;

    // read the environment map with the reflection vector
    vec3 reflectedColor = textureCube(u_fragmentCubeMap0, globalReflect).rgb;

#if 0 // TDM
    // calculate fresnel reflectance.
    float q = 1.0 - dotEN;
    float fresnel = 3.0 * q * q * q * q;
    reflectedColor *= (fresnel + 0.4);

    // tonemap to convert HDR values to range 0.0 - 1.0
    gl_FragColor = vec4(reflectedColor / (vec3(1.0) + reflectedColor), 1.0) * u_glColor * var_Color;
#else // original DOOM3
    gl_FragColor = vec4(reflectedColor, 1.0)/* * u_glColor * var_Color */;
#endif
}
