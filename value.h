#pragma once

#include "common.h"

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
    } as;
} Value;

Value boolVal(bool value);
Value nilVal();
Value numberVal(double value);

typedef void (*ValueFn)();

bool asBool(Value value);
double asNumber(Value value);

bool isBool(Value value);
bool isNil(Value value);
bool isNumber(Value value);


typedef struct
{
    int capacity;
    int count;
    Value *values;
} ValueArray;

void initValueArray(ValueArray *array);
void writeValueArray(ValueArray *array, Value value);
void freeValueArray(ValueArray *array);
void printValue(Value value);
bool valuesEqual(Value a, Value b);