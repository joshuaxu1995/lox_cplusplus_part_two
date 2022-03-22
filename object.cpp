#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

static ObjString* allocateString(char* chars, int length) {
    ObjString* string = allocateObj<ObjString>(OBJ_STRING);
    string->length = length;
    string->chars = chars;
    return string;
}

ObjString* copyString(const char* chars, int length) {
    char* heapChars = allocate<char>(length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length);
}

ObjString* takeString(char* chars, int length) {
    return allocateString(chars, length);
}

void printObject(Value value) {
    switch (objType(value)) {
        case OBJ_STRING:
            printf("%s", asCstring(value));
            break;
    }
}

static Obj* allocateObject(size_t size, ObjType type){
    Obj* object = (Obj*) reallocate(NULL, 0, size);
    object->type = type;

    object->next = vm.objects;
    vm.objects = object;
    return object;
}

ObjType objType(Value value){
    return asObj(value)->type;
}

bool isString(Value value){
    return isObjType(value, OBJ_STRING);
}

ObjString* asString(Value value){
    return (ObjString*) asObj(value);
}

char* asCstring(Value value){
    return asString(value)->chars;
}