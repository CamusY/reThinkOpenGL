#ifndef SHADER_H
#define SHADER_H

#include "../Interface/IShader.h"


class Shader : public IShader {
public:

	Shader(const char* vertexPath, const char* fragmentPath);

	void use() override;

	void setBool(const std::string& name, bool value) const override;
	void setInt(const std::string& name, int value)const override;
	void setFloat(const std::string& name, float value)const override;

private:
	//shader程序ID
	unsigned int ID;
};
#endif