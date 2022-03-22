#include <stdio.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value.h"

Value objVal(Obj* obj){
    return (Value) {VAL_OBJ, {.obj = obj}};
}

Value boolVal(bool val){
    return (Value) {VAL_BOOL, {.boolean = val}};
}

Value nilVal(){
    return (Value) {VAL_NIL, {.number = 0}};
}

Value numberVal(double val){
    return (Value) {VAL_NUMBER, {.number = val}};
}

Obj* asObj(Value value){
    return value.as.obj;
}

bool asBool(Value value){
    return value.as.boolean;
}
double asNumber(Value value){
    return value.as.number;
}

bool isObj(Value value){
    return value.type == VAL_OBJ;
}

bool isBool(Value value){
    return value.type == VAL_BOOL;
}

bool isNil(Value value){
    return value.type == VAL_NIL;
}

bool isNumber(Value value){
    return value.type == VAL_NUMBER;
}

void initValueArray(ValueArray *array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ValueArray *array, Value value)
{
    if (array->capacity < array->count + 1)
    {
        int oldCapacity = array->capacity;
        array->capacity = growCapacity(oldCapacity);
        array->values = growArray<Value>(array->values, oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray *array)
{
    freeArray<Value>(array->values, array->capacity);
    initValueArray(array);
}

void printValue(Value value)
{
    switch(value.type) {
        case VAL_BOOL:
            printf(asBool(value) ? "true" : "false");
            break;
        case VAL_NIL:
            printf("nil");
            break;
        case VAL_NUMBER:
            printf("%g", asNumber(value));
            break;
        case VAL_OBJ:
            printObject(value);
            break;
    }
}

bool valuesEqual(Value a, Value b){
    if (a.type != b.type){
        return false;
    }
    switch (a.type) {
        case VAL_BOOL:
            return asBool(a) == asBool(b);
        case VAL_NIL:
            return true;
        case VAL_NUMBER:
            return asNumber(a) == asNumber(b);
        case VAL_OBJ: {
            ObjString* aString = asString(a);
            ObjString* bString = asString(b);
            return aString->length == bString->length &&
                memcmp(aString->chars, bString->chars, aString->length) == 0;
        }
        default: 
            return false;
    }
}