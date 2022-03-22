#include <string.h>

#include "vm.h"
#include "compiler.h"
#include "common.h"
#include "debug.h"

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
}

static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}

void initVM() {
    resetStack();
    vm.objects = NULL;
    initTable(&vm.strings);
}

void freeVM() {
    freeTable(&vm.strings);
    freeObjects();
}

void push(Value value){
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

Value peek(int distance){
    Value val = vm.stackTop[-1 - distance];
    return val;
}

static bool isFalsey(Value value) {
    return isNil(value) || (isBool(value) && !asBool(value));
}

static void concatenate() {
    ObjString* b = asString(pop());
    ObjString* a = asString(pop());
    int length = a->length + b->length;
    char* chars = allocate<char>(length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    push(objVal((Obj*) result));
}

u_int8_t readByte(){
    return *vm.ip++;;
}

Value readConstant(){
    return vm.chunk->constants.values[readByte()];
}

void debugTraceExecution(){
    printf("      ");
    for (Value* slot = vm.stack; slot <vm.stackTop; slot++){
        printf("[ ");
        printValue(*slot);
        printf(" ]");
    }
    printf("\n");
    disassembleInstruction(vm.chunk, (int) (vm.ip - vm.chunk->code));
}

template<typename T>
InterpretResult binaryOp(ValueType type, T func){
    while (true){
        if (!isNumber(peek(0)) || !isNumber(peek(1))){
            runtimeError("Operands must be numbers.");
            return INTERPRET_RUNTIME_ERROR;
        }
        double b = asNumber(pop());
        double a = asNumber(pop());
        //TODO: Change depending on the type
        if (type == VAL_BOOL){
            push(boolVal(func(a,b)));
        } else if (type == VAL_NUMBER){
            push(numberVal(func(a,b)));
        }
        break;
    }
    return INTERPRET_OK;
}

struct Add
{
    double operator() (double l, double r){
        return l + r;
    }
};

struct Subtract
{
    double operator() (double l, double r){
        return l - r;
    }
};

struct Multiply
{
    double operator() (double l, double r){
        return l * r;
    }
};

struct Divide
{
    double operator() (double l, double r){
        return l / r;
    }
};

struct GreaterThan
{
    bool operator() (double l, double r){
        return l > r;
    }
};

struct LessThan
{
    bool operator() (double l, double r){
        return l < r;
    }
};

static InterpretResult run() {
    for (;;){
        debugTraceExecution();
        u_int8_t instruction;
        switch(instruction = readByte()){
            case OP_RETURN: {
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
            }
            case OP_CONSTANT: {
                Value constant = readConstant();
                push(constant);
                printValue(constant);
                printf("\n");
                break;
            }
            case OP_NIL:
                push(nilVal());
                break;
            case OP_TRUE:
                push(boolVal(true));
                break;
            case OP_FALSE:
                push(boolVal(false));
                break;
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(boolVal(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER:
                if (binaryOp(VAL_BOOL, GreaterThan()) == INTERPRET_RUNTIME_ERROR){
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            case OP_LESS:
                if (binaryOp(VAL_BOOL, LessThan()) == INTERPRET_RUNTIME_ERROR){
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            case OP_ADD: {
                if (isString(peek(0)) && isString(peek(1))){
                    concatenate();
                } else if (isNumber(peek(0)) && isNumber(peek(1))){
                    double b = asNumber(pop());
                    double a = asNumber(pop());
                    push(numberVal(a + b));
                } else{
                    runtimeError("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: {
                if (binaryOp(VAL_NUMBER, Subtract()) == INTERPRET_RUNTIME_ERROR){
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_MULTIPLY: {
                if (binaryOp(VAL_NUMBER, Multiply()) == INTERPRET_RUNTIME_ERROR){
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_DIVIDE: {
                if (binaryOp(VAL_NUMBER, Divide()) == INTERPRET_RUNTIME_ERROR){
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_NOT: {
                push(boolVal(isFalsey(pop())));
                break;
            }
            case OP_NEGATE: {
                if (!isNumber(peek(0))){
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(numberVal(-asNumber(pop())));
                break;
            }
        }
    }
}


InterpretResult interpret(const char* source) {
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    freeChunk(&chunk);
    return result;
}

