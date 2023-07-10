#ifndef _PATCHCACHE_H_
#define _PATCHCACHE_H_

#include <stdint.h>

void init_patch_cache();
uint8_t patch_cache_pattern(char *pattern, uint32_t *addrOut);

#endif