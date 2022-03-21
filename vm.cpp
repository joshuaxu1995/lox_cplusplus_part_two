#include "vm.h"
#include "compiler.h"
#include "common.h"
#include "debug.h"

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
}

void initVM() {
    resetStack();
}

void freeVM() {

}

void push(Value value){
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
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
void binaryOp(T func){
    while (true){
        double b = pop();
        double a = pop();
        push(func(a,b));
        break;
    }
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
            case OP_ADD: {
                binaryOp(Add());
                break;
            }
            case OP_SUBTRACT: {
                binaryOp(Subtract());
                break;
            }
            case OP_MULTIPLY: {
                binaryOp(Multiply());
                break;
            }
            case OP_DIVIDE: {
                binaryOp(Divide());
                break;
            }
            case OP_NEGATE: {
                push(-pop());
                break;
            }
        }
    }
}


InterpretResult interpret(const char* source) {
    compile(source);
    return INTERPRET_OK;
}

