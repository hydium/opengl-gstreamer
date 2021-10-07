// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

#include <common/shader.hpp> 

#include <gst/gst.h>
#include <gst/app/gstappsink.h>


#include <chrono>
#include <ctime>

// #include <iterator>
#include <iostream>

#include <vector>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

using namespace std;

int main( void )
{
	const int height = 1080;
	const int width = 1920;


	// GLubyte* image = (GLubyte*) malloc(height * width * 3);


	// int i, j;
	// uint8_t c;

	// for (i = 0; i < height; i++) {
	// 	for (j = 0; j < width; j++) {
	// 		c = ((((i&0x8)==0)^((j&0x8))==0))*255;

	// 		*(image + ( i * width * 3 + j * 3)) = (GLubyte) c;
	// 		*(image + ( i * width * 3 + j * 3 + 1)) = (GLubyte) c;
	// 		*(image + ( i * width * 3 + j * 3 + 2)) = (GLubyte) c;
	// 	}
	// }


	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( width, height, "GStreamer appsink", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Dark blue background
	glClearColor(0.0f, 1.0f, 0.0f, 0.0f);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders( "SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader" );



	// static const GLfloat g_vertex_buffer_data[] = { 
	// 	-1.0f, -1.0f, 0.0f,   0.0f, 1.0f,
	// 	 1.0f, -1.0f, 0.0f,   1.0f, 1.0f,
	// 	-1.0f,  1.0f, 0.0f,	  0.0f, 0.0f,

	// 	 1.0f,  1.0f, 0.0f,	  1.0f, 0.0f,
	// 	 1.0f, -1.0f, 0.0f,	  1.0f, 1.0f,
	// 	-1.0f,  1.0f, 0.0f,   0.0f, 0.0f,
	// };

	// GLfloat vertices[] = { 
	// 	-1.0f, -1.0f, 0.0f,
	// 	 1.0f, -1.0f, 0.0f,
	// 	-1.0f,  1.0f, 0.0f,
	// 	 1.0f,  1.0f, 0.0f,
	// };


	// GLuint indices[] = {  // note that we start from 0!
 //    	0, 1, 2,   // first triangle
 //    	1, 2, 3    // second triangle
	// }; 



	const int cols = 256;
	const int rows = 256;


	// cout << 128.0f / (float) rows << endl;

	GLfloat g_vertex_buffer_data[rows * cols * 3];

	for (int col = 0; col < cols; col++)
	{
		for (int row = 0; row < rows; row++)
		{
			g_vertex_buffer_data[3 * (col * rows + row)] = (float) col / (float) (cols - 1) * 2 - 1.0f;
			g_vertex_buffer_data[3 * (col * rows + row) + 1] = (float) row / (float) (rows - 1) * 2 - 1.0f;
			g_vertex_buffer_data[3 * (col * rows + row) + 2] = 0;
		}
	}



	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * rows * cols* 3, g_vertex_buffer_data, GL_STATIC_DRAW);



	GLfloat g_uv_buffer_data[rows * cols * 2];

	for (int col = 0; col < cols; col++)
	{
		for (int row = 0; row < rows; row++)
		{
			g_uv_buffer_data[2 * (col * rows + row)] = (float) col / (float) cols ;
			g_uv_buffer_data[2 * (col * rows + row) + 1] = (float) row / (float) rows;
		}
	}


	//distortion correction by changing uv coordinates
	for (int col = 0; col < cols; col++)
	{
		for (int row = 0; row < rows; row++)
		{
			float x = g_uv_buffer_data[2 * (col * rows + row)] ;
			float y = g_uv_buffer_data[2 * (col * rows + row) + 1];

			x = x * 2.0f - 1.0f;
			y = y * 2.0f - 1.0f;

			float xyz_[3] ;

			xyz_[0] = x;
			xyz_[1] = y;
			xyz_[2] = 1;


			// mat3 xyz = make_mat3(xyz_);
			tvec3<float> xyz = make_vec3(xyz_);


			// float cameraMatrix_[3][3] = {{753.349186340502,	0,	1010.31833065182},
			// 						  {0,	753.143587767122,	588.647123579411},
			// 						  {0,	0,	1}};

			float cameraMatrix_[9] = {753.349186340502,	0,	1010.31833065182,
									  0,	753.143587767122,	588.647123579411,
									  0,	0,	1};

			

			mat3 cameraMatrix = transpose(make_mat3(cameraMatrix_));



			// float one_[3] = {0, 0, 1};

			// tvec3<float> one = make_vec3(one_);

			// one = cameraMatrix * one;

			// cout << one[0] << endl;
			// cout << one[1] << endl;
			// cout << one[2] << endl;
			
			

			xyz = xyz * inverse(cameraMatrix);

			float r2 = pow(xyz[0], 2) + pow(xyz[1], 2);

			double dist[5] = {-0.358074811139381, 0.150366096279157, -0.000239617440106,	-0.001364488806427,	-0.031502910462795};

			// xyz = xyz / (1 + r2 * dist[0] + pow(r2, 2) * dist[1] + pow(r2, 3) * dist[4]);

			// tvec3<float> xyz1;

			xyz[0] = xyz[0] / (1 + r2 * dist[0] + pow(r2, 2) * dist[1] + pow(r2, 3) * dist[4]);
			xyz[1] = xyz[1] / (1 + r2 * dist[0] + pow(r2, 2) * dist[1] + pow(r2, 3) * dist[4]);

			xyz = xyz * cameraMatrix;
 			
 			g_uv_buffer_data[2 * (col * rows + row)] = xyz[0];
 			g_uv_buffer_data[2 * (col * rows + row) + 1] = xyz[1];

		}
	}



	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * rows * cols * 2, g_uv_buffer_data, GL_STATIC_DRAW);


	vector<GLuint> indices((cols - 1) * (rows - 1) * 6);
	


	int index = 0;


	for (int col = 0; col < cols - 1; col++)
	{
		for (int row = 0; row < rows - 1; row++)
		{
			indices[index] = col * rows + row;
			indices[index + 1] = col * rows + row + 1;
			indices[index + 2] = (col + 1) * rows + row;
			indices[index + 3] = (col + 1) * rows + row;
			indices[index + 4] = col * rows + row + 1;
			indices[index + 5] = (col + 1) * rows + row + 1;

			index += 6;
		}
	}


	GLuint elementbuffer;
	glGenBuffers(1, &elementbuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);


	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// glGenerateMipmap(GL_TEXTURE_2D);
	// glEnable(GL_TEXTURE_2D);



	// glEnableVertexAttribArray(0);
	// glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	// glVertexAttribPointer(
	// 	0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
	// 	3,                  // size
	// 	GL_FLOAT,           // type
	// 	GL_FALSE,           // normalized?
	// 	5 * sizeof(float),  // stride
	// 	(void*)0            // array buffer offset
	// );

	// glEnableVertexAttribArray(1);
	// glVertexAttribPointer(
	// 	1,                  // attribute 1. No particular reason for 1, but must match the layout in the shader.
	// 	2,                  // size
	// 	GL_FLOAT,           // type
	// 	GL_FALSE,           // normalized?
	// 	5 * sizeof(float),  // stride
	// 	(void*) (3 * sizeof(float))            // array buffer offset
	// );


	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		3 * sizeof(float),  // stride
		(void*)0            // array buffer offset
	);




	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glVertexAttribPointer(
		1,                  // attribute 1. No particular reason for 1, but must match the layout in the shader.
		2,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		2 * sizeof(float),  // stride
		(void*)0            // array buffer offset
	);





	//gstreamer
	GstElement *pipeline, *appsink;
	GstSample *sample; 
	GError *error = NULL;
	
	gst_init (NULL, NULL);


	pipeline = gst_parse_launch("v4l2src device=/dev/video2 ! image/jpeg, width=1920, height=1080, pixel-aspect-ratio=1/1, framerate=30/1 ! jpegdec ! videoconvert ! video/x-raw, width=1920, height=1080, format=\"RGB\" ! appsink name=appsink", &error);
	

	if (error != NULL) {
    	g_print ("could not construct pipeline: %s\n", error->message);
    	g_error_free (error);
    	return -1;
  	}

	appsink = gst_bin_get_by_name (GST_BIN (pipeline), "appsink");

	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	
	GstBuffer *prev_buffer = NULL;
	GstSample *prev_sample = NULL;	


	auto last_second = std::chrono::high_resolution_clock::now();

	int fps = 0;

	do{
		auto current = std::chrono::high_resolution_clock::now();

		if (std::chrono::duration_cast<std::chrono::milliseconds>(current - last_second).count() > 1000)
		{
			last_second = current;
			printf("fps: %d\n", fps);
			fps = 0;
		}

		fps++;

		// Clear the screen
		glClear( GL_COLOR_BUFFER_BIT );


		sample = gst_app_sink_pull_sample (GST_APP_SINK (appsink));
		
	
		if (sample == NULL) {
			g_print("Rip, no frame\n");
			continue;
		}
		
		GstBuffer *buffer = gst_sample_get_buffer (sample);
		GstMapInfo map;
		gst_buffer_map ( buffer, &map, GST_MAP_READ);


		// Use our shader
		glUseProgram(programID);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, map.data);

		// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

		glBindTexture(GL_TEXTURE_2D, texture);

		// Draw the triangle !
		// glDrawArrays(GL_TRIANGLES, 0, 6); // 3 indices starting at 0 -> 1 triangle
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (void*) 0);


		if (prev_sample) {
			gst_buffer_unmap(prev_buffer, &map);
			gst_sample_unref(prev_sample);
		}
		
		prev_buffer = buffer;
		prev_sample = sample;

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	// Cleanup VBO
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteVertexArrays(1, &VertexArrayID);
	glDeleteProgram(programID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

	return 0;
}

