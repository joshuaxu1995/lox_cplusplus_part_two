#include "memory.h"
#include "vm.h"
#include "compiler.h"

const int GC_HEAP_GROW_FACTOR = 2;

int growCapacity(int capacity)
{
    return capacity < 8 ? 8 : (capacity * 2);
}

static void markRoots() {
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        markValue(*slot);
    }

    for (int i = 0; i < vm.frameCount; i++){
        markObject((Obj*) vm.frames[i].closure);
    }
    
    for (ObjUpvalue* upvalue = vm.openUpvalues; upvalue != NULL;
        upvalue = upvalue->next){
        markObject((Obj*) upvalue);
    }

    markTable(&vm.globals);
    markCompilerRoots();
}

void markValue(Value value) {
    if (isObj(value)) {
        markObject(asObj(value));
    }
}

static void markArray(ValueArray* array){
    for (int i = 0; i < array->count; i++){
        markValue(array->values[i]);
    }
}

static void blackenObject(Obj* object) {

    if (DEBUG_LOG_GC) {
        printf("%p blacken ", (void*) object);
        printValue(objVal(object));
        printf("\n");
    }
    switch (object->type) {
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*) object;
            markObject((Obj*) closure->function);
            for (int i = 0; i < closure->upvalueCount; i++) {
                markObject((Obj*) closure->upvalues[i]);
            }
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*) object;
            markObject((Obj*) function->name);
            markArray(&function->chunk.constants);
            break;
        }
        case OBJ_UPVALUE: {
            markValue(((ObjUpvalue*) object)->closed);
            break;
        }
        case OBJ_NATIVE:
        case OBJ_STRING:
            break;
    }
}

static void traceReferences() {
    while (vm.grayCount > 0) {
        Obj* object = vm.grayStack[--vm.grayCount];
        blackenObject(object);
    }
}

static void freeObject(Obj* object) {
    if (DEBUG_LOG_GC){
        printf("%p free type %d\n", (void*) object, object->type);
    }
    switch (object->type) {
         case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*) object;
            freeArray<ObjUpvalue*>(closure->upvalues, closure->upvalueCount);
            free<ObjUpvalue>((ObjUpvalue*) object);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*) object;
            freeChunk(&function->chunk);
            free<Obj>(object);
            break;
        }
        case OBJ_NATIVE: {
            free<ObjNative>((ObjNative*) object);
            break;
        }
        case OBJ_STRING: {
            ObjString* string = (ObjString*) object;
            freeArray<char>(string->chars, string->length + 1);
            free<Obj>(object);
            break;
        }
        case OBJ_UPVALUE: {
            free<ObjUpvalue>((ObjUpvalue*) object);
        }
    }
}


static void sweep() {
    Obj* previous = NULL;
    Obj* object = vm.objects;
    while (object != NULL) {
        if (object->isMarked) {
            object->isMarked = false;
            previous = object;
            object = object->next;
        } else{
            Obj* unreached = object;
            object = object->next;
            if (previous != NULL) {
                previous->next = object;
            } else{
                vm.objects = object;
            }

            freeObject(unreached);
        }
    }
}

void collectGarbage() {
    size_t before = vm.bytesAllocated;
    if (DEBUG_LOG_GC){
        printf("-- gc begin\n");
    }

    markRoots();
    traceReferences();
    tableRemoveWhite(&vm.strings);
    sweep();

    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;

    if (DEBUG_LOG_GC){
        printf("--gc end\n");
        printf("  collected %zu bytes (from %zu to %zu) next at %zu\n",
            before - vm.bytesAllocated, before, vm.bytesAllocated, vm.nextGC);  
    }
}

void markObject(Obj* object) {
    if (object == NULL) {
        return;
    }
    if (object->isMarked) {
        return;
    }
    if (DEBUG_LOG_GC) {
        printf("%p mark ", (void*) object);
        printValue(objVal(object));
        printf("\n");
    }
    object->isMarked = true;

    if (vm.grayCapacity < vm.grayCount + 1) {
        vm.grayCapacity = growCapacity(vm.grayCapacity);
        vm.grayStack = (Obj**) realloc(vm.grayStack, sizeof(Obj*) * vm.grayCapacity);
        if (vm.grayStack == NULL) {
            exit(1);
        }
    }
    vm.grayStack[vm.grayCount++] = object;
}

void *reallocate(void *pointer, size_t oldSize, size_t newSize)
{
    vm.bytesAllocated += newSize - oldSize;
    if (newSize > oldSize) {
        if (DEBUG_STRESS_GC){
            collectGarbage();
        }

        if (vm.bytesAllocated > vm.nextGC) {
            collectGarbage();
        }
    }

    if (newSize == 0)
    {
        free(pointer);
        return NULL;
    }

    void *result = realloc(pointer, newSize);
    if (result == NULL)
    {
        exit(1);
    }
    return result;
}

void freeObjects() {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }

    free(vm.grayStack);
}
