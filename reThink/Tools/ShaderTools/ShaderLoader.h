#ifndef SHADERLOADER_H
#define SHADERLOADER_H

#include <string>

class ShaderLoader {
public:
	std::string LoadShaderCode(const std::string& shaderPath);
};

#endif