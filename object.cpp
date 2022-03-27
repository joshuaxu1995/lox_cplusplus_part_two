#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"
#include "table.h"

static ObjString* allocateString(char* chars, int length, uint32_t hash) {
    ObjString* string = allocateObj<ObjString>(OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    tableSet(&vm.strings, string, nilVal());
    return string;
}

static uint32_t hashString(const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++){
        hash ^= (uint8_t) key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString* copyString(const char* chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);

    if (interned != NULL){
        return interned;
    }

    char* heapChars = allocate<char>(length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
}

ObjString* takeString(char* chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);

    if (interned != NULL) {
        freeArray<char>(chars, length + 1);
        return interned;
    }

    return allocateString(chars, length, hash);
}

static void printFunction(ObjFunction* function){
    if (function->name == NULL){
        printf("<script>");
        return;
    }
    printf("<fun %s>", function->name->chars);
}

void printObject(Value value) {
    switch (objType(value)) {
        case OBJ_FUNCTION:
            printFunction(asFunction(value));
            break;
        case OBJ_NATIVE:
            printf("<native fn>");
            break;
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

ObjFunction* newFunction() {
    ObjFunction* function = allocateObj<ObjFunction>(OBJ_FUNCTION);
    function->arity = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

ObjNative* newNative(NativeFn function) {
    ObjNative* native = allocateObj<ObjNative>(OBJ_NATIVE);
    native->function = function;
    return native;
}

ObjType objType(Value value){
    return asObj(value)->type;
}

bool isNative(Value value){
    return isObjType(value, OBJ_NATIVE);
}

NativeFn asNative(Value value){
    return ((ObjNative*) asObj(value))->function;
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

bool isFunction(Value value){
    return isObjType(value, OBJ_FUNCTION);
}

ObjFunction* asFunction(Value value){
    return (ObjFunction*) asObj(value);
}