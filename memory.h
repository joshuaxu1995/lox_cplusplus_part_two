#pragma once

#include "common.h"
#include <iostream>

int growCapacity(int capacity);

void *reallocate(void *pointer, size_t oldSize, size_t newSize);

template <typename T>
T *freeArray(T *pointer, int oldCount)
{
    return (T *)reallocate(pointer, sizeof(T) * oldCount, 0);
}

template <typename T>
T *growArray(T *pointer, int oldCount, int newCount)
{
    return (T *)reallocate(pointer, sizeof(T) * oldCount, sizeof(T) * newCount);
}