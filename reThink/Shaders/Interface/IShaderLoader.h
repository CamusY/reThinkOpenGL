#ifndef ISHADERLOADER_H
#define SIHADERLOADER_H

#include <string>

class IShaderLoader {
public:
	virtual std::string LoadShaderCode(const std::string& shaderPath) = 0;
	virtual ~IShaderLoader() = default;
};
#endif