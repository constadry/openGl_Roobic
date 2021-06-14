#version 330 core

layout(location = 2) in vec3 vertexColor;
layout(location = 3) in vec3 right_vec;

out vec3 fragmentColorRight;

uniform mat4 model;

void main(){	

	// Output position of the vertex, in clip space : MVP * position
	gl_Position =  model * vec4(right_vec,1);

	// The color of each vertex will be interpolated
	// to produce the color of each fragment
	fragmentColorRight = vertexColor;
}