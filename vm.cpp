#include <string.h>
#include <time.h>
#include "vm.h"
#include "compiler.h"
#include "common.h"
#include "debug.h"
#include "vmdata.pb.h"
#include "serialize.h"
#include <fstream>


std::string arrayForPrinting[] = {
    "OP_CONSTANT",
    "OP_NIL",
    "OP_TRUE",
    "OP_FALSE",
    "OP_POP",
    "OP_GET_LOCAL",
    "OP_SET_LOCAL",
    "OP_GET_GLOBAL",
    "OP_DEFINE_GLOBAL",
    "OP_SET_GLOBAL",
    "OP_EQUAL",
    "OP_GREATER",
    "OP_LESS",
    "OP_ADD",
    "OP_SUBTRACT",
    "OP_MULTIPLY",
    "OP_DIVIDE",
    "OP_NOT",
    "OP_NEGATE",
    "OP_PRINT",
    "OP_JUMP",
    "OP_JUMP_IF_FALSE",
    "OP_LOOP",
    "OP_CALL",
    "OP_RETURN"
};

VM vm;

static Value clockNative(int argCount, Value* args) {
    return numberVal((double) clock() / CLOCKS_PER_SEC);
}

static void resetStack() {
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
}

static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);
    for (int i = vm.frameCount - 1; i >= 0; i--){
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        int line = function->chunk.lines[instruction];
        fprintf(stderr, "[line %d] in script\n", line);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else{
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }
    resetStack();
}

static void defineNative(const char* name, NativeFn function) {
    push(objVal((Obj*) copyString(name, (int)strlen(name))));
    push(objVal((Obj*) newNative(function)));
    tableSet(&vm.globals, asString(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void initVM() {
    resetStack();
    vm.objects = NULL;
    initTable(&vm.globals);
    initTable(&vm.strings);

    defineNative("clock", clockNative);
}

void freeVM() {
    freeTable(&vm.globals);
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

static bool call(ObjClosure* closure, int argCount) {
    if (argCount != closure->function->arity){
        runtimeError("Expected %d arguments but got %d.", 
            closure->function->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAMES_MAX){
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

static bool callValue(Value callee, int argCount) {
    if (isObj(callee)){
        switch (objType(callee)){
            case OBJ_CLOSURE:
                return call(asClosure(callee), argCount);
            case OBJ_NATIVE: {
                NativeFn native = asNative(callee);
                Value result = native(argCount, vm.stackTop - argCount);
                vm.stackTop -= argCount + 1;
                push(result);
                return true;
            }
            default:
                break;
        }
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

static ObjUpvalue* captureUpvalue(Value* local) {
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue = vm.openUpvalues;
    while (upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue* createdUpvalue = newUpvalue(local);
    if (prevUpvalue == NULL) {
        vm.openUpvalues = createdUpvalue;
    } else{
        prevUpvalue->next = createdUpvalue;
    }
    return createdUpvalue;
}

static void closeUpvalues(Value* last) {
    while (vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
        ObjUpvalue* upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.openUpvalues = upvalue->next;
    }
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
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    return *frame->ip++;
}

Value readConstant(){
    u_int8_t byteNum = readByte();
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    return frame->closure->function->chunk.constants.values[byteNum];
}

u_int16_t readShort(){
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    frame->ip += 2;
    uint8_t instructPointer = *frame->ip;
    uint16_t firstValue = frame->ip[-2];
    uint16_t firstValueShifted = frame->ip[-2] << 8;
    uint16_t secondValue = frame->ip[-1];
    uint16_t result = frame->ip[-2] << 8 | frame->ip[-1];
    return (uint16_t) (result);
}

ObjString* readString(){
    Value value = readConstant();
    return asString(value);
}

void debugTraceExecution(){
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    printf("      ");
    for (Value* slot = vm.stack; slot <vm.stackTop; slot++){
        printf("[ ");
        printValue(*slot);
        printf(" ]");
    }
    printf("\n");
    disassembleInstruction(&frame->closure->function->chunk, (int) (frame->ip - 
        frame->closure->function->chunk.code));
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
    CallFrame* frame = &vm.frames[vm.frameCount - 1];

    for (;;){
        // debugTraceExecution();
        u_int8_t instruction = readByte();
        // std::cout << "Printing instruction string value: " << arrayForPrinting[instruction] << std::endl;
        switch(instruction){
            case OP_PRINT: {
                Value value = pop();
                printValue(value);
                printf("\n");
                break;
            }
            case OP_LOOP: {
                uint16_t offset = readShort();
                frame->ip -= offset;
                break;
            }
            case OP_CALL: {
                int argCount = readByte();
                if (!callValue(peek(argCount), argCount)){
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_CLOSURE: {
                ObjFunction* function = asFunction(readConstant());
                ObjClosure* closure = newClosure(function);
                push(objVal((Obj*) closure));
                for (int i = 0; i < closure->upvalueCount; i++){
                    uint8_t isLocal = readByte();
                    uint8_t index = readByte();
                    if (isLocal) {
                        closure->upvalues[i] = captureUpvalue(frame->slots + index);
                    } else{
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }
            case OP_RETURN: {
                Value result = pop();
                closeUpvalues(frame->slots);
                vm.frameCount--;
                if (vm.frameCount == 0) {
                    pop();
                    return INTERPRET_OK;
                }

                vm.stackTop = frame->slots;
                push(result);
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_CONSTANT: {
                Value constant = readConstant();
                push(constant);
                // printValue(constant);
                // printf("\n");
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
            case OP_POP:
                pop();
                break;
            case OP_GET_LOCAL: {
                uint8_t slot = readByte();
                // std::cout << "Reading slot: " << +slot << std::endl;
                push(frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = readByte();
                frame->slots[slot] = peek(0);
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString* name = readString();
                Value value;
                if (!tableGet(&vm.globals, name, &value)) {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString* name = readString();
                tableSet(&vm.globals, name, peek(0));
                pop();
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* name = readString();;
                if (tableSet(&vm.globals, name, peek(0))){
                    tableDelete(&vm.globals, name);
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_UPVALUE: {
                uint8_t slot = readByte();
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
            case OP_SET_UPVALUE: {
                uint8_t slot = readByte();
                *frame->closure->upvalues[slot]->location = peek(0);
                break;
            }
            case OP_CLOSE_UPVALUE: {
                closeUpvalues(vm.stackTop - 1);
                pop();
                break;
            }
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
            case OP_JUMP: {
                uint16_t offset = readShort();
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = readShort();
                if (isFalsey(peek(0))){
                    frame->ip += offset;
                }
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
    ObjFunction* function = compile(source);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    push(objVal((Obj*) function));
    ObjClosure* closure = newClosure(function);
    pop();
    push(objVal((Obj*) closure));
    call(closure, 0);

    serializationPackage::VMData vmData = serializeVMData(vm, locationOfFunctions, locationsOfNonInstructions);
    std::fstream output("VMDataFile.txt", std::ios::out | std::ios::trunc | std::ios::binary);
    if (!vmData.SerializeToOstream(&output)) {
      std::cerr << "Failed to write vmdata to file." << std::endl;
      return INTERPRET_COMPILE_ERROR;
    }

    return run();

    // if (__cplusplus == 201703L) std::cout << "C++17\n";
    // else if (__cplusplus == 201402L) std::cout << "C++14\n";
    // else if (__cplusplus == 201103L) std::cout << "C++11\n";
    // else if (__cplusplus == 199711L) std::cout << "C++98\n";
    // else std::cout << "pre-standard C++\n";
    

    // freeChunk(&chunk);


    // return result;
    // return INTERPRET_OK;
}

