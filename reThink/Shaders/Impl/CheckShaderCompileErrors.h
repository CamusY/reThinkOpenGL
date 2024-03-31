#ifndef CHECKSHADERCOMPILEERRORS_H
#define CHECKSHADERCOMPILEERRORS_H

#include "../Interface/ICheckCompileErrors.h"

#include <string>
class CheckShaderCompileErrors : public ICheckCompileErrors {
public:
	void checkCompileErrors(unsigned int shader,std::string type);
};

#endif // !CHECKSHADERCOMPILEERRORS_H

