#version 140

out float ViewSpaceZ;

uniform sampler2D u_depthTexture;

uniform block {
	mat4 u_projectionMatrix;
};

float nearZ = -0.5 * u_projectionMatrix[3][2];

void main() {
	float depth = texelFetch(u_depthTexture, ivec2(gl_FragCoord.xy), 0).r;
	ViewSpaceZ = nearZ / (depth + 0.5 * (u_projectionMatrix[2][2] - 1));
}
