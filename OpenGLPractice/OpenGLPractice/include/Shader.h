#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h> 

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>


class Shader {
public:
	unsigned int ShaderProgramID;
	Shader(const char* vertexPath, const char* fragmentPath);
	void use();
private:
	void checkCompileErrors(unsigned int shader, std::string type);
};


#endif