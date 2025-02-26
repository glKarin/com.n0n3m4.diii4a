//this shader is a light shader. ideally drawn with a quad covering the entire region
//the output is contribution from this light (which will be additively blended)
//you can blame Electro for much of the maths in here.
!!ver 100 450
//FIXME: !!permu FOG
!!samps shadowmap 2

#include "sys/defs.h"
#include "sys/pcf.h"

//s_t0 is the depth
//s_t1 is the normals+spec-exponent
//output should be amount of light hitting the surface.

varying vec4 tf;
#ifdef VERTEX_SHADER
void main()
{
	tf = ftetransform();
	gl_Position = tf;
}
#endif
#ifdef FRAGMENT_SHADER

#define out_diff fte_fragdata0
#define out_spec fte_fragdata1

vec3 calcLightWorldPos(vec2 screenPos, float depth)
{
	vec4 pos = m_invviewprojection * vec4(screenPos.xy, (depth*2.0)-1.0, 1.0);
	return pos.xyz / pos.w;
}
void main ()
{
	vec3 lightColour		= l_lightcolour.rgb;

	vec2 fc = tf.xy / tf.w;
	vec2 gc = (1.0 + fc) / 2.0;
	float depth = texture2D(s_t0, gc).r;
	vec4 data = texture2D(s_t1, gc);
	vec3 norm = data.xyz;
	float spec_exponent = data.a;

	/* calc where the wall that generated this sample came from */
	vec3 worldPos	= calcLightWorldPos(fc, depth);

	/*we need to know the cube projection (for both cubemaps+shadows)*/
	vec4 cubeaxis = l_cubematrix*vec4(worldPos.xyz, 1.0);

	/*calc ambient lighting term*/
	vec3 lightDir = l_lightposition - worldPos;
	float atten = max(1.0 - (dot(lightDir, lightDir)/(l_lightradius*l_lightradius)), 0.0);

	/*calc diffuse lighting term*/
	lightDir = normalize(lightDir);
	float nDotL = dot(norm, lightDir);
	float lightDiffuse = max(0.0, nDotL);

	/*calc specular lighting term*/
	vec3 halfdir = normalize(normalize(e_eyepos - worldPos) + lightDir);	//ASSUMPTION: e_eyepos requires an identity modelmatrix (true for world+sprites, but usually not for models/bsps)
	float spec = pow(max(dot(halfdir, norm), 0.0), spec_exponent);

	//fixme: apply fog?
	//fixme: cubemap filters

	float shadows = ShadowmapFilter(s_shadowmap, cubeaxis);

	out_diff = vec4(lightColour * (l_lightcolourscale.x + l_lightcolourscale.y*lightDiffuse*shadows), 1.0);
	out_spec = vec4(lightColour * l_lightcolourscale.z*spec*shadows, 1.0);
}
#endif
