#version 300 es
//#pragma optimize(off)

precision mediump float;

#define ConvertYCoCgToRGB(x) x.rgb

uniform sampler2D u_fragmentMap0;
uniform sampler2D u_fragmentMap1;
uniform sampler2D u_fragmentMap2;
uniform sampler2D u_fragmentMap3;
uniform sampler2D u_fragmentMap4;
uniform sampler2D u_fragmentMap5;
uniform sampler2D u_fragmentMap6;
uniform sampler2D u_fragmentMap7;

in vec4 var_TexCoord0;
in vec4 var_TexCoord1;
in vec4 var_TexCoord2;
in vec4 var_TexCoord3;
in vec4 var_TexCoord4;
in vec4 var_TexCoord5;
in vec4 var_TexCoord6;
in vec4 var_TexCoord7;

out vec4 _gl_FragColor;

void main(void)
{
    vec4 mask1, mask2;
    vec4 invMask1, invMask2;
    vec4 combined;
    vec2 scaled;
    vec4 R2;

    mask1.x = texture(u_fragmentMap0, var_TexCoord0.xy).x;
    mask1.y = texture(u_fragmentMap0, var_TexCoord1.xy).x;
    mask1.z = texture(u_fragmentMap0, var_TexCoord2.xy).x;
    mask1.w = texture(u_fragmentMap0, var_TexCoord3.xy).x;

    mask2.x = texture(u_fragmentMap0, var_TexCoord4.xy).x;
    mask2.y = texture(u_fragmentMap0, var_TexCoord5.xy).x;
    mask2.z = texture(u_fragmentMap0, var_TexCoord6.xy).x;
    mask2.w = 1.0;

    invMask1 = 1.0 - mask1;
    invMask2 = 1.0 - mask2;

    combined = vec4(0.0);
    scaled = var_TexCoord7.zw;

    // Sample the lowest quality first.
    R2 = texture(u_fragmentMap1, scaled.xy);
    R2.xyz = ConvertYCoCgToRGB(R2);
    scaled = scaled * 2.0;
    combined = combined * invMask1.x;
    combined = (R2 * mask1.x) + combined;

    R2 = texture(u_fragmentMap2, scaled.xy);
    R2.xyz = ConvertYCoCgToRGB(R2);
    scaled = scaled * 2.0;
    combined = combined * invMask1.y;
    combined = (R2 * mask1.y) + combined;

    R2 = texture(u_fragmentMap3, scaled.xy);
    R2.xyz = ConvertYCoCgToRGB(R2);
    scaled = scaled * 2.0;
    combined = combined * invMask1.z;
    combined = (R2 * mask1.z) + combined;

    R2 = texture(u_fragmentMap4, scaled.xy);
    R2.xyz = ConvertYCoCgToRGB(R2);
    scaled = scaled * 2.0;
    combined = combined * invMask1.w;
    combined = (R2 * mask1.w) + combined;

    R2 = texture(u_fragmentMap5, scaled.xy);
    R2.xyz = ConvertYCoCgToRGB(R2);
    scaled = scaled * 2.0;
    combined = combined * invMask2.x;
    combined = (R2 * mask2.x) + combined;

    R2 = texture(u_fragmentMap6, scaled.xy);
    R2.xyz = ConvertYCoCgToRGB(R2);
    scaled = scaled * 2.0;
    combined = combined * invMask2.y;
    combined = (R2 * mask2.y) + combined;

    R2 = texture(u_fragmentMap7, scaled.xy);
    R2.xyz = ConvertYCoCgToRGB(R2);
    scaled = scaled * 2.0;
    combined = combined * invMask2.z;
    combined = (R2 * mask2.z) + combined;

    _gl_FragColor = combined;
}
