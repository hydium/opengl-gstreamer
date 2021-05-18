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



	static const GLfloat g_vertex_buffer_data[] = { 
		-1.0f, -1.0f, 0.0f,   0.0f, 1.0f,
		 1.0f, -1.0f, 0.0f,   1.0f, 1.0f,
		-1.0f,  1.0f, 0.0f,	  0.0f, 0.0f,

		 1.0f,  1.0f, 0.0f,	  1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,	  1.0f, 1.0f,
		-1.0f,  1.0f, 0.0f,   0.0f, 0.0f,
	};


	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// glGenerateMipmap(GL_TEXTURE_2D);
	// glEnable(GL_TEXTURE_2D);



	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		5 * sizeof(float),  // stride
		(void*)0            // array buffer offset
	);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1,                  // attribute 1. No particular reason for 1, but must match the layout in the shader.
		2,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		5 * sizeof(float),  // stride
		(void*) (3 * sizeof(float))            // array buffer offset
	);




	//gstreamer
	GstElement *pipeline, *appsink;
	GstSample *sample; 
	GError *error = NULL;
	
	gst_init (NULL, NULL);


	pipeline = gst_parse_launch("v4l2src device=/dev/video0 ! image/jpeg, width=1920, height=1080, pixel-aspect-ratio=1/1, framerate=60/1 ! jpegdec ! videoconvert ! video/x-raw, width=1920, height=1080, format=\"RGB\" ! appsink name=appsink", &error);
	

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
		glDrawArrays(GL_TRIANGLES, 0, 6); // 3 indices starting at 0 -> 1 triangle


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

