#ifndef ICHECKCOMPILEERRORS_H
#define ICHECKCOMPILEERRORS_H

#include <string>

class ICheckCompileErrors {
public:
	virtual void checkCompileErrors(unsigned int ID,std::string type) = 0;
	virtual ~ICheckCompileErrors() = default;
};
#endif // !ICHECKCOMPILEERRORS_H
