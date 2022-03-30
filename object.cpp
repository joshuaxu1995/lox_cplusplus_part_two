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

    push(objVal((Obj*) string));
    tableSet(&vm.strings, string, nilVal());
    pop();
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

ObjUpvalue* newUpvalue(Value* slot) {
    ObjUpvalue* upvalue = allocateObj<ObjUpvalue>(OBJ_UPVALUE);
    upvalue->closed = nilVal();
    upvalue->location = slot;
    upvalue->next =NULL;
    return upvalue;
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
        case OBJ_CLASS:
            printf("%s", asClass(value)->name->chars);
            break;
        case OBJ_CLOSURE:
            printFunction(asClosure(value)->function);
            break;
        case OBJ_FUNCTION:
            printFunction(asFunction(value));
            break;
        case OBJ_INSTANCE:
            printf("%s instance", asInstance(value)->klass->name->chars);
            break;
        case OBJ_NATIVE:
            printf("<native fn>");
            break;
        case OBJ_STRING:
            printf("%s", asCstring(value));
            break;
        case OBJ_UPVALUE:
            printf("upvalue");
            break;
    }
}

static Obj* allocateObject(size_t size, ObjType type){
    Obj* object = (Obj*) reallocate(NULL, 0, size);
    object->type = type;
    object->isMarked = false;

    object->next = vm.objects;
    vm.objects = object;

    if (DEBUG_LOG_GC){
        printf("%p allocate %zu for %d\n", (void*) object, size, type);
    }

    return object;
}

ObjInstance* newInstance(ObjClass* klass){
    ObjInstance* instance = allocateObj<ObjInstance>(OBJ_INSTANCE);
    instance->klass = klass;
    initTable(&instance->fields);
    return instance;
}

ObjClass* newClass(ObjString* name) {
    ObjClass* klass = allocateObj<ObjClass>(OBJ_CLASS);
    klass->name = name;
    return klass;
}

ObjClosure* newClosure(ObjFunction* function){
    ObjUpvalue** upvalues = allocate<ObjUpvalue*>(function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }
    ObjClosure* closure = allocateObj<ObjClosure>(OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

ObjFunction* newFunction() {
    ObjFunction* function = allocateObj<ObjFunction>(OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
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

bool isInstance(Value value){
    return isObjType(value, OBJ_INSTANCE);
}

ObjInstance* asInstance(Value value){
    return ((ObjInstance*) asObj(value));
}

bool isClass(Value value) {
    return isObjType(value, OBJ_CLASS);
}

ObjClass* asClass(Value value) {
    return ((ObjClass*) asObj(value));
}

bool isClosure(Value value){
    return isObjType(value, OBJ_CLOSURE);
}

ObjClosure* asClosure(Value value){
    return ((ObjClosure*) asObj(value));
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