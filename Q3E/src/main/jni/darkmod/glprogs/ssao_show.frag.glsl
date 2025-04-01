#version 140

// display the occlusion channel from the SSAO texture for debug purposes

in vec2 var_TexCoord;
out vec3 FragColor;

uniform sampler2D u_source;

void main() {
	float ssao = texture(u_source, var_TexCoord).r;
	FragColor = vec3(ssao, ssao, ssao);
}
