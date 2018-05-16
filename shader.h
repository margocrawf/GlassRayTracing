#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if defined(__APPLE__)
#include <GLUT/GLUT.h>
#include <OpenGL/gl3.h>
#include <OpenGL/glu.h>
#else
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#endif
#include <GL/glew.h>		 
#include <GL/freeglut.h>	
#endif


void checkShader(unsigned int shader, char* message);

void checkLinking(unsigned int program);

class Shader
{
protected:
	unsigned int shaderProgram;

public:
	Shader()
	{
		const char *vertexSource = "\n\
			#version 130 \n\
			precision highp float; \n\
			\n\
			in vec2 vertexPosition;	\n\
			in vec2 vertexTexCoord; \n\
			out vec2 texCoord; \n\
			\n\
			void main() \n\
			{ \n\
				texCoord = vertexTexCoord; \n\
				gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1); \n\
			} \n\
		"; 

		const char *fragmentSource = "\n\
			#version 130 \n\
			precision highp float; \n\
			\n\
			uniform sampler2D samplerUnit; \n\
			in vec2 texCoord;  \n\
			out vec4 fragmentColor; \n\
			\n\
			void main() { \n\
			fragmentColor = texture(samplerUnit, texCoord);  \n\
			} \n\
		";

		unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
		if (!vertexShader) { printf("Error in vertex shader creation\n"); exit(1); }

		glShaderSource(vertexShader, 1, &vertexSource, NULL);
		glCompileShader(vertexShader);
		checkShader(vertexShader, "Vertex shader error");

		unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		if (!fragmentShader) { printf("Error in fragment shader creation\n"); exit(1); }

		glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
		glCompileShader(fragmentShader);
		checkShader(fragmentShader, "Fragment shader error");

		shaderProgram = glCreateProgram();
		if (!shaderProgram) { printf("Error in shader program creation\n"); exit(1); }

		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);

		glBindAttribLocation(shaderProgram, 0, "vertexPosition");
		glBindAttribLocation(shaderProgram, 1, "vertexTexCoord");

		glBindFragDataLocation(shaderProgram, 0, "fragmentColor");

		glLinkProgram(shaderProgram);
		checkLinking(shaderProgram);
	}

	~Shader()
	{
		if(shaderProgram) glDeleteProgram(shaderProgram);
	}

	void Run()
	{
		if(shaderProgram) glUseProgram(shaderProgram);
	}

	void UploadSamplerID()
	{
		int samplerUnit = 0; 
		int location = glGetUniformLocation(shaderProgram, "samplerUnit");
		glUniform1i(location, samplerUnit);
		glActiveTexture(GL_TEXTURE0 + samplerUnit); 
	}
};
