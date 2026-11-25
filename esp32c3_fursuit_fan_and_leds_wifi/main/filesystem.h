#pragma once
#include "main.h"

bool filesystem_init();
void* filesystem_readFileToHeap(const char* path);
void* filesystem_readFileToHeapAndGetSize(const char* path, size_t* size);
bool filesystem_writeFile(const char* path, void* data, size_t size);
bool filesystem_exists(const char* path);