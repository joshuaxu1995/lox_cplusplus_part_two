#pragma once

#include "value.h"

typedef enum {
    OBJ_STRING
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;
};

struct ObjString {
    Obj obj;
    int length;
    char* chars;
};

static Obj* allocateObject(size_t size, ObjType type);

template <typename T>
T *allocateObj(ObjType objType)
{
    return (T *) allocateObject(sizeof(T), objType);
}


static inline bool isObjType(Value value, ObjType type){
    return isObj(value) && asObj(value)->type == type;
}

ObjType objType(Value value);
bool isString(Value value);
ObjString* asString(Value value);
char* asCstring(Value value);
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
void printObject(Value value);