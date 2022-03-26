#pragma once

#include "vm.h"
#include "object.h"

ObjFunction* compile(const char* source);
extern std::vector<ObjFunction*> locationOfFunctions;