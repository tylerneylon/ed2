// memprofile.h
//
// https://github.com/tylerneylon/cstructs
//

#pragma once

void *memop(char *file, int line, void *ptr, int numBytes, int isRealloc);
void printmeminfo();

#if 1

#define malloc(numBytes) memop(__FILE__, __LINE__, NULL, (int)numBytes, 0)
#define realloc(oldPtr, numBytes) \
    memop(__FILE__, __LINE__, oldPtr, (int)numBytes, 1)
#define free(ptr) memop(__FILE__, __LINE__, ptr, -1, 0)

#endif
