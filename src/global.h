#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <stdint.h>

#define filePathBufLen 1024
char *executableDirectory[filePathBufLen];

void *base_addr;
uint32_t mod_size;

#endif