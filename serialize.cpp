#include "serialize.h"

void serializeInstructions(serializationPackage::VMData* vmData, Chunk* chunk){
    int hi = 32;
    // std::cout<< "INITIAL: Printing vmDatamap instructions size: " << vmData->instructions().size()
    //         << std::endl;
    for (int i = 0; i < chunk->count; i++){
        uint8_t* addressOfFirstEntry = chunk->code + i;
        uint64_t printedValueAddress = (uint64_t) addressOfFirstEntry;
        // std::cout<< "Printing addressoffirstentry: " << printedValueAddress << std::endl;
        uint8_t actualValue = *addressOfFirstEntry;
        vmData->add_instructions(actualValue);
        // std::cout<< "Printing vmDatamap instructions size: " << vmData->instructions().size()
            // << std::endl;
    }
}

void serializeConstants(serializationPackage::VMData* vmData, Chunk* chunk){
    auto& vmDataMap = *(vmData->mutable_constantvals());
    for (int i = 0; i < chunk->constants.count; i++){
        Value* valuePointer = chunk->constants.values + i;
        Value myValue = *valuePointer;
        serializationPackage::ValueType valueType;
        if (isBool(myValue)){
            // valueType.set
            valueType.set_boolval(asBool(myValue));
        } else if (isNumber(myValue)){
            valueType.set_numval(asNumber(myValue));
        } else if (isObj(myValue)){
            valueType.set_objectaddress((uintptr_t) myValue.as.obj);
        }
        vmDataMap[i] = valueType;
        int hi = 32;
    }
}

void serializeStrings(serializationPackage::VMData* vmData, Obj* obj){
    auto& vmDataMap = *(vmData->mutable_stringsataddresses());
    while (obj != NULL){
        ObjString* tempString = (ObjString*) obj;
        serializationPackage::VMData_AddressAndHash addressAndHash;
        addressAndHash.set_address(reinterpret_cast<uintptr_t>(obj));
        addressAndHash.set_hash(tempString->hash);
        vmDataMap[tempString->chars] = addressAndHash;
        obj = obj->next;
    }
}

serializationPackage::VMData serializeVMData(VM vm){
    serializationPackage::VMData vmdata;
    serializationPackage::ValueType valueType;
    serializeConstants(&vmdata, vm.chunk);
    serializeStrings(&vmdata, vm.objects);
    serializeInstructions(&vmdata, vm.chunk);
    std::cout<< vmdata.DebugString();
    return vmdata;
}
