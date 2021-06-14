// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include "shader.hpp"

struct Point {
	float x, y, z;
};

void bind_cube_buf(static const GLfloat g_buffer_data[], GLuint &buffer) {
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_buffer_data) * 12 * 3 * 3, g_buffer_data, GL_STATIC_DRAW);
}

void set_coord(static GLfloat g_vertex_buffer_data[27][12 * 3 * 3], const int i, const Point& point) {
	for (int v = 0; v < 12 * 3 * 3; v += 3) {
		g_vertex_buffer_data[i][v] = g_vertex_buffer_data[0][v] + point.x;
		g_vertex_buffer_data[i][v + 1] = g_vertex_buffer_data[0][v + 1] + point.y;
		g_vertex_buffer_data[i][v + 2] = g_vertex_buffer_data[0][v + 2] + point.z;
	}
}

void set_color(static GLfloat g_color_buffer_data[27][12 * 3 * 3], const int i, const Point& point) {
	for (int v = 0; v < 12 * 3 * 3; v += 3) {
		g_color_buffer_data[i][v] = point.x;
		g_color_buffer_data[i][v + 1] = point.y;
		g_color_buffer_data[i][v + 2] = point.z;
	}
}

void draw_cube(
		GLuint vertexbuffer[],
		GLuint colorbuffer[],
		const int i,
		GLuint vertexPosition_modelspaceID,
		GLuint vertexColorID
	) {

	glEnableVertexAttribArray(vertexPosition_modelspaceID);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[i]);
	glVertexAttribPointer(
		vertexPosition_modelspaceID, // attribute. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	// 2nd attribute buffer : colors
	glEnableVertexAttribArray(vertexColorID);
	glBindBuffer(GL_ARRAY_BUFFER, colorbuffer[i]);
	glVertexAttribPointer(
		vertexColorID,                    // attribute. No particular reason for 1, but must match the layout in the shader.
		3,                                // size
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		0,                                // stride
		(void*)0                          // array buffer offset
	);

	// Draw the triangle !
	glDrawArrays(GL_TRIANGLES, 0, 12 * 3); // 12*3 indices starting at 0 -> 12 triangles

}

int main(void)
{
	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		//getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1024, 768, "Colored Cube", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		//getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		//getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	GLuint VertexArrayID[2];
	glGenVertexArrays(1, &VertexArrayID[0]);
	glBindVertexArray(VertexArrayID[0]);
	
	glGenVertexArrays(1, &VertexArrayID[1]);
	glBindVertexArray(VertexArrayID[1]);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders("TransformVertexShader.vertexshader", "ColorFragmentShader.fragmentshader");

	// Get a handle for our "MVP" uniform
	GLuint MatrixID;
	MatrixID = glGetUniformLocation(programID, "MVP");

	//GLuint vertexPosition_modelspace_RightID = glGetUniformLocation(programID, "vertexPosition_modelspace_Right");
	//GLuint vertexPosition_modelspaceID = glGetUniformLocation(programID, "vertexPosition_modelspace");
	//GLuint vertexColorID = glGetUniformLocation(programID, "vertexColor");
	//GLuint vertexColorRightID = glGetUniformLocation(programID, "vertexColorRight");
	//

	// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
	// Camera matrix
	glm::mat4 View = glm::lookAt(
		glm::vec3(16, 6, 20), // Camera is at (4,3,-3), in World Space
		glm::vec3(0, 0, 0), // and looks at the origin
		glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
	);
	// Model matrix : an identity matrix (model will be at the origin)
	glm::mat4 Model = glm::mat4(1.0f);
	// Our ModelViewProjection : multiplication of our 3 matrices
	glm::mat4 MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around

	// Our vertices. Tree consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
	// A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices

	static const GLfloat g_vertex_buffer_data_f[] = {
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		 1.0f, 1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		 1.0f,-1.0f,-1.0f,
		 1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		 1.0f,-1.0f, 1.0f,
		 1.0f, 1.0f, 1.0f,
		 1.0f,-1.0f,-1.0f,
		 1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f,-1.0f,
		 1.0f, 1.0f, 1.0f,
		 1.0f,-1.0f, 1.0f,
		 1.0f, 1.0f, 1.0f,
		 1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		 1.0f,-1.0f, 1.0f
	};

	static GLfloat g_vertex_buffer_data[27][12 * 3 * 3];
	static GLfloat g_color_buffer_data[27][12 * 3 * 3];
	GLuint vertexbuffer[18];
	GLuint colorbuffer[18];

	for (int v = 0; v < 12 * 3 * 3; ++v) {
		g_vertex_buffer_data[0][v] = g_vertex_buffer_data_f[v];
	}
	
	set_coord(g_vertex_buffer_data, 1, { 0.f, -2.f, 0.f });
	set_coord(g_vertex_buffer_data, 2, { 0.f, 2.f, 0.f });
	set_coord(g_vertex_buffer_data, 3, { -2.f, 0.f, 0.f });
	set_coord(g_vertex_buffer_data, 4, { 2.f, 0.f, 0.f });

	set_coord(g_vertex_buffer_data, 5, { 0.f, 0.f, -2.f });
	set_coord(g_vertex_buffer_data, 6, { -2.f, 2.f, -2.f });
	set_coord(g_vertex_buffer_data, 7, { 0.f, 2.f, -2.f });
	set_coord(g_vertex_buffer_data, 8, { 2.f, 2.f, -2.f });
	set_coord(g_vertex_buffer_data, 9, { 2.f, 0.f, -2.f });
	set_coord(g_vertex_buffer_data, 10, { -2.f, 0.f, -2.f });
	set_coord(g_vertex_buffer_data, 11, { -2.f, -2.f, -2.f });
	set_coord(g_vertex_buffer_data, 12, { 0.f, -2.f, -2.f });
	set_coord(g_vertex_buffer_data, 13, { 2.f, -2.f, -2.f });

	set_coord(g_vertex_buffer_data, 14, { -2.f, 2.f, 0.f });
	set_coord(g_vertex_buffer_data, 15, { 2.f, 2.f, 0.f });
	set_coord(g_vertex_buffer_data, 16, { -2.f, -2.f, 0.f });
	set_coord(g_vertex_buffer_data, 17, { 2.f, -2.f, 0.f });

	set_coord(g_vertex_buffer_data, 18, { 0.f, 0.f, 2.f });
	set_coord(g_vertex_buffer_data, 19, { -2.f, 2.f, 2.f });
	set_coord(g_vertex_buffer_data, 20, { 0.f, 2.f, 2.f });
	set_coord(g_vertex_buffer_data, 21, { 2.f, 2.f, 2.f });
	set_coord(g_vertex_buffer_data, 22, { 2.f, 0.f, 2.f });
	set_coord(g_vertex_buffer_data, 23, { -2.f, 0.f, 2.f });
	set_coord(g_vertex_buffer_data, 24, { -2.f, -2.f, 2.f });
	set_coord(g_vertex_buffer_data, 25, { 0.f, -2.f, 2.f });
	set_coord(g_vertex_buffer_data, 26, { 2.f, -2.f, 2.f });
	
	Point colors[] = {
		{1.f, 0.f, 0.f},
		{0.f, 1.f, 0.f},
		{0.f, 0.f, 1.f},
		{0.5f, 0.5f, 0.f},
		{0.5f, 0.f, 0.5f},
		{0.f, 0.5f, 0.5f}
	};

	for (int v = 0; v < 27; ++v) {
		set_color(g_color_buffer_data, v, colors[v % 6]);
	}

	for (int v = 0; v < 18; ++v) {
		bind_cube_buf(g_vertex_buffer_data[v], vertexbuffer[v]);
		bind_cube_buf(g_color_buffer_data[v], colorbuffer[v]);
	}

	GLuint vertexbuffer_right[9];
	GLuint colorbuffer_right[9];

	for (int v = 0; v < 9; ++v) {
		bind_cube_buf(g_vertex_buffer_data[18 + v], vertexbuffer_right[v]);
		bind_cube_buf(g_color_buffer_data[18 + v], colorbuffer_right[v]);
	}
	glm::mat4 T;

	do {

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		glm::mat4 trans = glm::mat4(1.0f);
		glm::mat4 model = glm::rotate(trans, (float)glfwGetTime(), glm::vec3(0.f, 0.f, 1.f));
		trans = MVP;
		T = model;

		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		for (int v = 0; v < 18; ++v) {
			draw_cube
			(
				vertexbuffer,
				colorbuffer,
				v,
				0,
				1
			);
		}

		MVP = model * MVP;
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		for (int v = 0; v < 9; ++v) {
			draw_cube
			(
				vertexbuffer_right,
				colorbuffer_right,
				v,
				0,
				1
			);
		}

		MVP = trans;
		model = T;

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(3);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

	// Cleanup VBO and shader
	for (int v = 0; v < 18; ++v) {
		glDeleteBuffers(1, &vertexbuffer[v]);
		glDeleteBuffers(1, &colorbuffer[v]);
	}

	for (int v = 0; v < 9; ++v) {
		glDeleteBuffers(1, &vertexbuffer_right[v]);
		glDeleteBuffers(1, &colorbuffer_right[v]);
	}
	
	glDeleteProgram(programID);
	//glDeleteProgram(prID_rot);
	glDeleteVertexArrays(1, &VertexArrayID[0]);
	glDeleteVertexArrays(1, &VertexArrayID[1]);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

