#pragma once

#include "vm.h"
#include "object.h"
#include <unordered_map>
#include <set>

ObjFunction* compile(const char* source);
extern std::vector<ObjFunction*> locationOfFunctions;
extern std::unordered_map<std::string, std::set<uint64_t>> locationsOfNonInstructions;
