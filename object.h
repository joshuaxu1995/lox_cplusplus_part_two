#pragma once

#include "chunk.h"
#include "value.h"

typedef enum {
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_UPVALUE
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;
};

typedef struct {
    Obj obj;
    int arity;
    int upvalueCount;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

typedef Value (*NativeFn) (int argCount, Value* args);

typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative;

struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash;
};

typedef struct ObjValue {
    Obj obj;
    Value* location;
} ObjUpvalue;

typedef struct {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int upvalueCount;
} ObjClosure;

ObjClosure* newClosure(ObjFunction* function);
ObjFunction* newFunction();
ObjNative* newNative(NativeFn function);
static Obj* allocateObject(size_t size, ObjType type);

template <typename T>
T *allocateObj(ObjType objType)
{
    return (T *) allocateObject(sizeof(T), objType);
}


static inline bool isObjType(Value value, ObjType type){
    return isObj(value) && asObj(value)->type == type;
}


bool isClosure(Value value);
ObjClosure* asClosure(Value value);
bool isNative(Value value);
NativeFn asNative(Value value);
bool isFunction(Value value);
ObjFunction* asFunction(Value value);
ObjType objType(Value value);
bool isString(Value value);
ObjString* asString(Value value);
char* asCstring(Value value);
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
ObjUpvalue* newUpvalue(Value* slot);
void printObject(Value value);