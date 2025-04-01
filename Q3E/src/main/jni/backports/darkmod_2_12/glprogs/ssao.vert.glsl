#version 330

out vec2 var_TexCoord;
out vec2 var_ViewRayXY;

uniform block {
	mat4 u_projectionMatrix;
};
vec2 halfTanFov = vec2(1 / u_projectionMatrix[0][0], 1 / u_projectionMatrix[1][1]);

void main() {
	var_TexCoord.x = gl_VertexID == 1 ? 2 : 0;
	var_TexCoord.y = gl_VertexID == 2 ? 2 : 0;
	gl_Position = vec4(var_TexCoord * 2 - 1, 1, 1);

	// prepare a part of the NDC to view space coordinate math here in the vertex shader to save instructions in the fragment shader
	var_ViewRayXY = -halfTanFov * (2 * var_TexCoord - 1);
}
