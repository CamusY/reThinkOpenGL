#ifndef ISHADER_H
#define ISHADER_H

#include <string>

class IShader {
public:

	virtual void use() = 0;

	virtual void setBool(const std::string& name, bool value) const = 0;
	virtual void setInt(const std::string& name, int value)const = 0;
	virtual  void setFloat(const std::string& name, float value)const = 0;
	virtual ~IShader() = default;

	IShader() = default;
};


#endif // !SHADER_H

