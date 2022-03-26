#include "serialize.h"
#include <typeinfo>

void serializeInstructions(serializationPackage::VMData* vmData, Chunk* chunk){
    // int hi = 32;
    // // std::cout<< "INITIAL: Printing vmDatamap instructions size: " << vmData->instructions().size()
    // //         << std::endl;
    // for (int i = 0; i < chunk->count; i++){
    //     uint8_t* addressOfFirstEntry = chunk->code + i;
    //     uint64_t printedValueAddress = (uint64_t) addressOfFirstEntry;
    //     // std::cout<< "Printing addressoffirstentry: " << printedValueAddress << std::endl;
    //     uint8_t actualValue = *addressOfFirstEntry;
    //     vmData->add_instructions(actualValue);
    //     // std::cout<< "Printing vmDatamap instructions size: " << vmData->instructions().size()
    //         // << std::endl;
    // }
}


void serializeConstantVals(serializationPackage::Context* context, ObjFunction* locationOfFunction){
    auto& contextConstantMap = *(context->mutable_constantvals());
    std::string name = context->contextname();
    for (int i = 0; i < locationOfFunction->chunk.constants.count; i++){
        Value* valuePointer = locationOfFunction->chunk.constants.values + i;
        Value myValue = *valuePointer;
        serializationPackage::ValueType valueType;
        if (isBool(myValue)){
            // valueType.set
            valueType.set_boolval(asBool(myValue));
        } else if (isNumber(myValue)){
            valueType.set_numval(asNumber(myValue));
        } else if (isObj(myValue) && isString(myValue)){
            valueType.set_stringaddress((uintptr_t) (ObjString*) myValue.as.obj);
        } else if (isObj(myValue) && isFunction(myValue)){
            valueType.set_functionaddress((uintptr_t) (ObjFunction*) myValue.as.obj);
        }
        contextConstantMap[i] = valueType;
        // int hi = 32;
    }
}

void serializeContexts(serializationPackage::VMData* vmData, std::vector<ObjFunction*> locationOfFunctions){
    for (auto& element: locationOfFunctions){
        serializationPackage::Context* context = vmData->add_contexts();
        context->set_functionaddress((uint64_t) element);
        if (element->name != NULL){
            context->set_contextname(element->name->chars);
            std::cout<< "Element here is: " << element << " and name: " << element->name->chars << " and arity: " << element->arity << std::endl;
        } else{
            context->set_contextname("");
            std::cout<< "Element here is: " << element << " and name: is script: " << " and arity: " << element->arity << std::endl;
        }
        context->set_arity(element->arity);
        auto& contextInstructionMap = *(context->mutable_instructionvals());
        for (int i = 0; i < element->chunk.count; i++){
            uint64_t address = (uint64_t) element->chunk.code + i;
            uint64_t opcodevalue_or_address = (*(element->chunk.code + i));
            contextInstructionMap[address] = opcodevalue_or_address;
            if (i == 0){
                context->set_firstinstructionaddress(address);
            }
            // context.mutable_instructionvals()->;
        }
        serializeConstantVals(context, element);
        std::cout<< "hi";
        // std::cout<< "Printing: " << vmData->mutable_contexts()->Add(context.);
    }
}

void serializeConstants(serializationPackage::VMData* vmData, Chunk* chunk){
    // auto& vmDataMap = *(vmData->mutable_constantvals());
    // for (int i = 0; i < chunk->constants.count; i++){
    //     Value* valuePointer = chunk->constants.values + i;
    //     Value myValue = *valuePointer;
    //     serializationPackage::ValueType valueType;
    //     if (isBool(myValue)){
    //         // valueType.set
    //         valueType.set_boolval(asBool(myValue));
    //     } else if (isNumber(myValue)){
    //         valueType.set_numval(asNumber(myValue));
    //     } else if (isObj(myValue)){
    //         valueType.set_objectaddress((uintptr_t) myValue.as.obj);
    //     }
    //     vmDataMap[i] = valueType;
    //     int hi = 32;
    // }
}

//Old way of serialization
// void serializeStrings(serializationPackage::VMData* vmData, Obj* obj){
//     auto& vmDataMap = *(vmData->mutable_stringsataddresses());
//     while (obj != NULL){
//         ObjString* tempString = (ObjString*) obj;
//         serializationPackage::VMData_AddressAndHash addressAndHash;
//         addressAndHash.set_address(reinterpret_cast<uintptr_t>(obj));
//         addressAndHash.set_hash(tempString->hash);
//         vmDataMap[tempString->chars] = addressAndHash;
//         obj = obj->next;
//     }
// }

void serializeStrings(serializationPackage::VMData* vmData, Table stringTable){
    auto& vmDataMap = *(vmData->mutable_stringsataddresses());
    for (int i = 0; i < stringTable.capacity; i++){
        Entry entry = stringTable.entries[i];
        if (entry.key != NULL){
            std::cout<<"Printing entry's string " << entry.key->chars << std::endl;
            serializationPackage::VMData_AddressAndHash addressAndHash;
            addressAndHash.set_address(reinterpret_cast<uintptr_t>(entry.key));
            addressAndHash.set_hash(entry.key->hash);
            vmDataMap[entry.key->chars] = addressAndHash;
        }
    }
}

serializationPackage::VMData serializeVMData(VM vm, std::vector<ObjFunction*> locationOfFunctions){
    serializationPackage::VMData vmdata;
    serializationPackage::ValueType valueType;
    serializeContexts(&vmdata, locationOfFunctions);
    // serializeConstants(&vmdata, vm.chunk);
    serializeStrings(&vmdata, vm.strings);
    // serializeInstructions(&vmdata, vm.chunk);
    std::cout<< vmdata.DebugString();
    return vmdata;
}
