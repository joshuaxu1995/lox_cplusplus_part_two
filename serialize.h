#pragma once

#include "vmdata.pb.h"
#include "vm.h"
#include "compiler.h"

serializationPackage::VMData serializeVMData(VM vmStateInfo, std::vector<ObjFunction*> locationOfFunctions, 
    std::unordered_map<std::string, std::set<uint64_t>> locationsOfNonInstructions,
    std::unordered_map<uint64_t, std::vector<Upvalue>> locationOfUpvalues);