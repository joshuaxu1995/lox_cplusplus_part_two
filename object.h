#pragma once
#include "chunk.h"
#include "table.h"

typedef enum {
    OBJ_BOUND_METHOD,
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_UPVALUE
} ObjType;

struct Obj {
    ObjType type;
    bool isMarked;
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

typedef struct ObjUpvalue {
    Obj obj;
    Value* location;
    Value closed;
    struct ObjUpvalue* next;
} ObjUpvalue;

typedef struct {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int upvalueCount;
} ObjClosure;

typedef struct {
    Obj obj;
    ObjString* name;
    Table methods;
} ObjClass;

typedef struct {
    Obj obj;
    ObjClass* klass;
    Table fields;
} ObjInstance;

typedef struct {
    Obj obj;
    Value receiver;
    ObjClosure* method;
} ObjBoundMethod;

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method);
ObjInstance* newInstance(ObjClass* klass);
ObjClass* newClass(ObjString* name);
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

bool isBoundMethod(Value value);
ObjBoundMethod* asBoundMethod(Value value);
bool isInstance(Value value);
ObjInstance* asInstance(Value value);
bool isClass(Value value);
ObjClass* asClass(Value value);
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