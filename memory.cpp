#include "memory.h"
#include "vm.h"

int growCapacity(int capacity)
{
    return capacity < 8 ? 8 : (capacity * 2);
}

void *reallocate(void *pointer, size_t oldSize, size_t newSize)
{
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

static void freeObject(Obj* object) {
    switch (object->type) {
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
    }
}

void freeObjects() {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }
}
