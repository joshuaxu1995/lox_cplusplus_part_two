#pragma once

#include "common.h"
#include "object.h"
#include <iostream>


int growCapacity(int capacity);

void *reallocate(void *pointer, size_t oldSize, size_t newSize);
void freeObjects();

template <typename T>
T *allocate(int count)
{
    return (T *)reallocate(NULL, 0, sizeof(T) * (count));
}

template <typename T>
T *freeArray(T *pointer, int oldCount)
{
    return (T *)reallocate(pointer, sizeof(T) * oldCount, 0);
}

template <typename T>
T *free(T *pointer)
{
    return (T *)reallocate(pointer, sizeof(T), 0);
}

template <typename T>
T *growArray(T *pointer, int oldCount, int newCount)
{
    return (T *)reallocate(pointer, sizeof(T) * oldCount, sizeof(T) * newCount);
}