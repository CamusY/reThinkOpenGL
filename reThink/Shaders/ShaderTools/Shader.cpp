#include "../Impl/Shader.h"
#include "../Impl/ShaderLoader.h"
#include "../Impl/CheckShaderCompileErrors.h"

#include <glad/glad.h>

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
	CheckShaderCompileErrors shaderCompileErrors;
	ShaderLoader loader;
	std::string vertexCode = loader.LoadShaderCode(vertexPath);
	std::string fragmentCode = loader.LoadShaderCode(fragmentPath);

	const char* vShaderCode = vertexCode.c_str();
	const char* fShaderCode = fragmentCode.c_str();

	unsigned int vertex, fragment;

	//顶点着色器
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vShaderCode, NULL);
	glCompileShader(vertex);
	shaderCompileErrors.checkCompileErrors(vertex, "VERTEX");

	//片段着色器
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fShaderCode, NULL);
	glCompileShader(fragment);
	shaderCompileErrors.checkCompileErrors(fragment, "FRAGMENT");

	ID = glCreateProgram();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);
	shaderCompileErrors.checkCompileErrors(ID, "PROGRAM");

	glDeleteShader(vertex);
	glDeleteShader(fragment);

}

void Shader::use(){
	glUseProgram(ID);
}

void Shader::setBool(const std::string& name, bool value) const{
	glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setInt(const std::string& name, int value) const{
	glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const{
	glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}