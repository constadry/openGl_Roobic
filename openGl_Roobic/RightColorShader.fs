#version 330 core

// Interpolated values from the vertex shaders
in vec3 fragmentColorRight;

// Ouput data
out vec3 colorRight;

void main(){

	// Output color = color specified in the vertex shader, 
	// interpolated between all 3 surrounding vertices
	colorRight = fragmentColorRight;

}