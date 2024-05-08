#version 150 core

in vec3 VertexPosition;
in float VertexStartTime;
in vec4 VertexColor;

out vec2 center;

uniform mat4 ciModelViewProjection;

void main() {
	gl_Position = ciModelViewProjection * vec4(VertexPosition, 1.0);

	gl_PointSize =5;

}