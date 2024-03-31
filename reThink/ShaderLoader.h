#ifndef SHADERLOADER_H
#define SHADERLOADER_H

#include "IShaderLoader.h"

class ShaderLoader : public IShaderLoader {
public:
	std::string LoadShaderCode(const std::string& shaderPath) override;
};

#endif // !SHADERLOADER_H
