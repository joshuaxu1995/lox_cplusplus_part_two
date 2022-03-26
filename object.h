#pragma once

#include "chunk.h"
#include "value.h"

typedef enum {
    OBJ_FUNCTION,
    OBJ_STRING
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;
};

typedef struct {
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash;
};

ObjFunction* newFunction();
static Obj* allocateObject(size_t size, ObjType type);

template <typename T>
T *allocateObj(ObjType objType)
{
    return (T *) allocateObject(sizeof(T), objType);
}


static inline bool isObjType(Value value, ObjType type){
    return isObj(value) && asObj(value)->type == type;
}

bool isFunction(Value value);
ObjFunction* asFunction(Value value);
ObjType objType(Value value);
bool isString(Value value);
ObjString* asString(Value value);
char* asCstring(Value value);
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
void printObject(Value value);