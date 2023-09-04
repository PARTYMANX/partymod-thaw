#include <patchcache.h>

#include <global.h>
#include <hash.h>
#include <patch.h>

map_t *patch_cache;

//#define PATTERN_FINDER_PARANOIA

void init_patch_cache() {
	patch_cache = map_alloc(64, NULL, NULL);

	// TODO: load file here
}

uint8_t patch_cache_pattern(char *pattern, uint32_t *addrOut) {
#ifdef PATTERN_FINDER_PARANOIA
	printf("checking cache...\n");
#endif
	uint32_t *cache_result = map_get(patch_cache, pattern, strlen(pattern));

	if (cache_result) {
#ifdef PATTERN_FINDER_PARANOIA
		printf("found!\n");
#endif
		*addrOut = *cache_result;
		return 1;
	}

	uint32_t find_result;
#ifdef PATTERN_FINDER_PARANOIA
	printf("finding %s...\n", pattern);
#endif
	if (findPattern(pattern, base_addr, mod_size, &find_result)) {
#ifdef PATTERN_FINDER_PARANOIA
		printf("found at 0x%08x! adding to cache\n", find_result);
#endif
		map_put(patch_cache, pattern, strlen(pattern), &find_result, sizeof(uint32_t));
#ifdef PATTERN_FINDER_PARANOIA
		printf("success\n");
#endif

		*addrOut = find_result;

		return 1;
	} else {
#ifdef PATTERN_FINDER_PARANOIA
		printf("shit!\n");
#endif
		return 0;
	}
}
