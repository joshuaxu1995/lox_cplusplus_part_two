#pragma once

#include "vm.h"
#include "object.h"
#include <unordered_map>
#include <set>

typedef struct {
    uint8_t index;
    bool isLocal;
} Upvalue;

ObjFunction* compile(const char* source);
extern std::vector<ObjFunction*> locationOfFunctions;
extern std::unordered_map<std::string, std::set<uint64_t>> locationsOfNonInstructions;
extern std::vector<Upvalue*> locationOfUpvalues;