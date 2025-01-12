#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <incbin/incbin.h>
#include <Windows.h>

#include <global.h>
#include <patch.h>
#include <patchcache.h>
#include <input.h>

#include <hash.h>

// GetPlatform: 8b 0d ?? ?? ?? ?? 33 c0 49 83 f9 07 0f 87 8e 00 00 00	// make sure to look back at this one to see how it works

INCBIN(groundtricks, "patches/groundtricks.bps");
INCBIN(levelselect_scripts, "patches/levelselect_scripts.bps");

char scriptCacheFile[1024];

uint8_t usingPS2Controls = 0;
uint8_t menuControls = 0;

map_t *cachedScriptMap;	// persistent cache, written to disk when modified
map_t *patchMap;
map_t *scriptMap;	// runtime cache, potentially modified scripts go in here.  think of it as a cache of buffers rather than scripts themselves

#define SCRIPT_CACHE_FILE_NAME "scriptCache.qbc"
#define CACHE_MAGIC_NUMBER 0x49676681 // QBC1

typedef struct {
	size_t filenameLen;
	char *filename;
	uint32_t checksum;	// this is the checksum of the patch this script was made against
	size_t scriptLen;
	uint8_t *script;
} scriptCacheEntry;

void saveScriptCache(char *filename) {
	FILE *file = fopen(filename, "wb");

	if (file) {
		uint32_t magicNum = CACHE_MAGIC_NUMBER;
		fwrite(&magicNum, sizeof(uint32_t), 1, file);
		fwrite(&cachedScriptMap->entries, sizeof(size_t), 1, file);
		printf("Saving %d entries\n", cachedScriptMap->entries);

		for (int i = 0; i < cachedScriptMap->len; i++) {
			struct bucketNode *node = cachedScriptMap->buckets[i].head;
			while (node) {
				scriptCacheEntry *entry = node->val;

				fwrite(&entry->filenameLen, sizeof(size_t), 1, file);
				fwrite(entry->filename, entry->filenameLen, 1, file);
				fwrite(&entry->checksum, sizeof(uint32_t), 1, file);
				fwrite(&entry->scriptLen, sizeof(size_t), 1, file);
				fwrite(entry->script, entry->scriptLen, 1, file);

				node = node->next;
			}
		}

		fclose(file);
	} else {
		printf("FAILED TO OPEN FILE!!\n");
	}
}

int loadScriptCache(char *filename) {
	FILE *file = fopen(filename, "rb");

	if (file) {
		uint32_t magicNum = 0;
		fread(&magicNum, sizeof(uint32_t), 1, file);

		if (magicNum != CACHE_MAGIC_NUMBER) {
			goto failure;
		}

		size_t entryCount = 0;
		if (!fread(&entryCount, sizeof(size_t), 1, file)) {
			goto failure;
		}
		printf("%d entries\n", entryCount);

		for (int i = 0; i < entryCount; i++) {
			scriptCacheEntry entry;

			if (!fread(&entry.filenameLen, sizeof(size_t), 1, file)) {
				goto failure;
			}

			entry.filename = calloc(entry.filenameLen + 1, 1);
			if (!entry.filename || !fread(entry.filename, entry.filenameLen, 1, file)) {
				goto failure;	
			}
			entry.filename[entry.filenameLen] = '\0';

			if (!fread(&entry.checksum, sizeof(uint32_t), 1, file)) {
				goto failure;
			}

			if (!fread(&entry.scriptLen, sizeof(size_t), 1, file)) {
				goto failure;
			}

			entry.script = malloc(entry.scriptLen);
			if (!entry.script || !fread(entry.script, entry.scriptLen, 1, file)) {
				goto failure;	
			}

			map_put(cachedScriptMap, entry.filename, entry.filenameLen, &entry, sizeof(entry));

			printf("Loaded cache entry for %s, patch checksum %08x\n", entry.filename, entry.checksum);
		}

		fclose(file);
		return 0;

	failure:
		printf("Failed to read script cache file!!  Cache may be corrupt and will be rewritten...\n");
		fclose(file);
		return -1;
	} else {
		printf("Script cache does not exist\n");
		return -1;
	}
}

int applyPatch(uint8_t *patch, size_t patchLen, uint8_t *input, size_t inputLen, uint8_t **output, size_t *outputLen);
uint32_t getPatchChecksum(uint8_t *patch, size_t sz);

void registerPatch(char *name, unsigned int sz, char *data) {
	map_put(patchMap, name, strlen(name), data, sz);
	printf("Registered patch for %s\n", name);
}

void initScriptPatches() {
	sprintf(scriptCacheFile, "%s%s", executableDirectory, SCRIPT_CACHE_FILE_NAME);

	patchMap = map_alloc(16, NULL, NULL);
	cachedScriptMap = map_alloc(16, NULL, NULL);
	scriptMap = map_alloc(16, NULL, NULL);

	registerPatch("pak\\levelselect\\levelselect_scripts.qb.wpc", glevelselect_scriptsSize, glevelselect_scriptsData);

	printf("Loading script cache from %s...\n", scriptCacheFile);
	loadScriptCache(scriptCacheFile);
	printf("Done!\n");
}

uint32_t (__cdecl *their_crc32)(char *) = (void *)0x00401b00;
uint32_t (__cdecl *addr_parseqbsecondfunc)(uint32_t, char *) = (void *)0x0040a8f0;
void (__cdecl *addr_parseqbthirdfunc)(uint8_t *, uint8_t *, void *, uint32_t, int) = (void *)0x0040a110;
void *addr_parseqb = NULL;
void *addr_unkparseqbfp = NULL;	// function pointer passed into third func
void *addr_isps2 = NULL;
void *addr_isxenon = NULL;
void *addr_ispc = NULL;
void *addr_isce = NULL;
void *addr_cfunclist = NULL;
void *addr_cfunccount = NULL;
void *addr_finishloads = NULL;
void *addr_isLoadingLevel = NULL;
void *addr_setscreenelementprops = NULL;
void *addr_playmovie = NULL;
uint32_t (__cdecl *addr_getPlatform)(uint8_t *, uint8_t *) = NULL;
uint8_t *addr_metabuttonmap = NULL;

// fixes crashes when going to menu
uint8_t __cdecl finishloadsWrapper(uint8_t *idk, uint8_t *script) {
	uint8_t (__cdecl *their_finishloads)(uint8_t *, uint8_t *) = addr_finishloads;
	uint8_t *isLoadingLevel = addr_isLoadingLevel;

	uint32_t scriptcrc = 0;

	if (script) {
		scriptcrc = *(uint32_t *)(script + 0xd0);
		//printf("FINISH LOADING FROM 0x%08x!\n", scriptcrc);
	}

	if (scriptcrc == 0x39c58ea1 || scriptcrc == 0x17b4f51c) {
		uint8_t tmp = *isLoadingLevel;
		*isLoadingLevel = 0;

		their_finishloads(idk, script);

		*isLoadingLevel = tmp;

		return 1;
	}

	return their_finishloads(idk, script);
}


uint8_t __cdecl isPs2Wrapper(uint8_t *idk, uint8_t *script) {
	uint32_t scriptcrc = 0;

	if (script) {
		scriptcrc = *(uint32_t *)(script + 0xd0);
		//printf("ISPS2 FROM 0x%08x!\n", scriptcrc);
	}

	// CAS menu
	if (scriptcrc == 0x2acdf8f2 || scriptcrc == 0x7410bd96) {
		if (menuControls == 2) {
			return 1;
		} else {
			return 0;
		}
	}

	if (usingPS2Controls && scriptcrc == 0x0fb58a23 || scriptcrc == 0xa6c34163) {
		// if BertStanceState or switchRegular, return true;
		return 1;
	}

	return 0;
}

uint8_t __cdecl isXenonWrapper(uint8_t *idk, uint8_t *script) {
	uint32_t scriptcrc = 0;

	if (script) {
		scriptcrc = *(uint32_t *)(script + 0xd0);
		//printf("ISXENON FROM 0x%08x!\n", scriptcrc);
	}

	// CAS menu
	if (scriptcrc == 0x2acdf8f2 || scriptcrc == 0x7410bd96) {
		if (menuControls == 1) {
			return 1;
		} else {
			return 0;
		}
	}

	return 0;
}

uint8_t __cdecl isPCWrapper(uint8_t *idk, uint8_t *script) {
	uint32_t scriptcrc = 0;

	if (script) {
		scriptcrc = *(uint32_t *)(script + 0xd0);
		//printf("ISPC FROM 0x%08x!\n", scriptcrc);
	}

	// CAS menu
	if (scriptcrc == 0x2acdf8f2 || scriptcrc == 0x7410bd96) {
		if (menuControls == 0) {
			return 1;
		} else {
			return 0;
		}
	}

	if (usingPS2Controls && scriptcrc == 0x711b0bee) {
		return 0;
	}

	return 1;
}

uint8_t __cdecl getPlatformWrapper(uint8_t *params, uint8_t *script) {
	uint32_t scriptcrc = 0;

	if (script) {
		scriptcrc = *(uint32_t *)(script + 0xd0);
		printf("GETPLATFORM FROM 0x%08x!\n", scriptcrc);
	}

	return addr_getPlatform(params, script);
}

uint8_t __cdecl isCEWrapper(uint8_t *idk, uint8_t *script) {
	// makes the game collector's edition 
	// NOTE: doesn't entirely work.  needs files for marseille and atlanta
	uint32_t scriptcrc = 0;

	if (script) {
		scriptcrc = *(uint32_t *)(script + 0xd0);
		//printf("IS COLLECTORS EDITION FROM 0x%08x!\n", scriptcrc);
	}

	return 1;
}

struct component {
	uint8_t unk;
	uint8_t type;
	uint16_t size;
	uint32_t name;
	void *data;
	struct component *next;
};

struct scrStruct {
	void *unk;
	struct component *head;
};

struct scrobjArray {
	void *unk;
	uint32_t size;
	void *data;
};

struct scrobjArray *scr_get_array(struct scrStruct *str, uint32_t checksum) {
	struct scrobjArray *result = NULL;

	struct component *node = str->head;
	while (node && node->name != checksum) {
		node = node->next;
	}
	if (node) {
		// get array
		result = node->data;
	}

	return result;
}

uint8_t __cdecl SetScreenElementPropsWrapper(uint8_t *params, uint8_t *script) {
	uint8_t (__cdecl *their_func)(uint8_t *, uint8_t *) = addr_setscreenelementprops;

	uint32_t scriptcrc = 0;

	if (script) {
		scriptcrc = *(uint32_t *)(script + 0xd0);
		//printf("SET SCREEN ELEMENTS PROP FROM 0x%08x!\n", scriptcrc);
	}

	// create_speech_box
	if (scriptcrc == 0xE0C4DA1A) {	// TODO: check setting
		// find event_handlers object 
		struct component *node = ((struct scrStruct *)params)->head;
		while (node && node->name != 0x475BF03C) {
			node = node->next;
		}
		if (node) {
			// get array
			struct scrobjArray *arr = node->data;
			for (int i = 0; i < arr->size; i++) {
				struct component *type = ((struct scrStruct **)arr->data)[i]->head;
			
				if (type->data == 0xCB5C11FA) {	// pad_option
					type->data = 0x456D7433;	// change to pad_circle
				} else if (type->data == 0x456D7433) {	// pad_circle
					type->data = 0xCB5C11FA;	// pad_option
				}
			}
		}
	}

	return their_func(params, script);
}

#define BUTTON_A 0x174841bc
#define BUTTON_B 0x8e411006
#define BUTTON_X 0x7323e97c
#define BUTTON_Y 0x0424d9ea
#define BUTTON_CIRCLE 0x2b489a86
#define BUTTON_TRIANGLE 0x20689278
#define BUTTON_SQUARE 0x321c9756
#define BUTTON_L1 0xcd254066
#define BUTTON_R1 0xbb890e41
#define BUTTON_L2 0x542c11dc
#define BUTTON_R2 0x22805ffb
#define BUTTON_L3 0xa8e3e604
#define BUTTON_R3 0x54d6f6a5
#define BUTTON_Z 0x9d2d8850
#define BUTTON_ZL 0x43e94857
#define BUTTON_ZR 0xebfe2fc7
#define BUTTON_RT_HALF 0x63d80c45
#define BUTTON_LT_HALF 0x9fed1ce4
#define BUTTON_LT_FULL 0x84c88965
#define BUTTON_RT_FULL 0x78fd99c4
#define BUTTON_WHITE 0xbd30325b
#define BUTTON_BLACK 0x767a45d7
#define BUTTON_SELECT 0xb40d153f
#define BUTTON_BACK 0x92313ec8
#define BUTTON_NULL 0xda3403b0

#define MAP_GAMECUBE 0x99304478
#define MAP_PS2 0x988a3508
#define MAP_XBOX 0x87d839b8
#define MAP_XENON 0xf6a179c0

struct buttonmapping {
	uint32_t button;
	uint32_t mapping;
};

// event map:
// get frontend class from SetButtonEventMappings + 3
//      *(uint *)(param_1 + 0xe4 + iVar4 * 8) = uVar7; = button index
//      *(int *)(param_1 + 0xe8 + iVar4 * 8) = iVar3; = event type
void __fastcall setButtonMappingsWrapper(void *frontend, void *pad, struct scrStruct *params) {
	//printf("SETTING BUTTONS 0x%08x\n", frontend);

	int map;
	switch(menuControls) {
		case 1:
			map = MAP_XENON;
			break;
		case 2:
			map = MAP_PS2;
			break;
		default:
			map = MAP_XBOX;
	}

	struct scrobjArray *button_mappings = scr_get_array(params, map);
	if (!button_mappings) {
		//printf("NO BUTTON MAPPINGS\n");
		return;
	}

	//printf("GOT ARRAY 0x%08x\n", params);
	struct buttonmapping *mappingarray = ((struct buttonmapping *)(((uint8_t *)frontend) + 0xe4));

	for (int i = 0; i < button_mappings->size; i++) {
		struct scrobjArray *button_map = ((struct scrobjArray **)button_mappings->data)[i];
		//printf("GOT STUFF 0x%08x\n", button_map);

		uint32_t button = ((uint32_t *)button_map->data)[0];
		uint32_t mapping = ((uint32_t *)button_map->data)[1];

		//printf("BUTTON: 0x%08x, MAPPING: 0x%08x\n", button, mapping);

		int button_idx = -1;

		switch(button) {
		case BUTTON_A:
			button_idx = 6;
			break;
		case BUTTON_TRIANGLE:
		case BUTTON_Y:
			button_idx = 4;
			break;
		case BUTTON_CIRCLE:
			button_idx = 5;
			break;
		case BUTTON_SQUARE:
			button_idx = 7;
			break;
		case BUTTON_X:
			if (map == MAP_PS2) {
				button_idx = 6;
			} else if (map == MAP_GAMECUBE) {
				button_idx = 5;
			} else {	// something xbox
				button_idx = 7;
			}
			break;
		case BUTTON_B:
			if (map == MAP_GAMECUBE) {
				button_idx = 7;
			} else {	// something xbox
				button_idx = 5;
			}
			break;
		case BUTTON_L1:
		case BUTTON_LT_HALF:
			button_idx = 2;
			break;
		case BUTTON_R1:
		case BUTTON_RT_HALF:
			button_idx = 3;
			break;
		case BUTTON_L2:
		case BUTTON_LT_FULL:
			button_idx = 0;
			break;
		case BUTTON_R2:
		case BUTTON_RT_FULL:
			button_idx = 1;
			break;
		case BUTTON_SELECT:
		case BUTTON_BACK:
			button_idx = 8;
			break;
		case BUTTON_ZL:
		case BUTTON_L3:
			button_idx = 9;
			break;
		case BUTTON_ZR:
		case BUTTON_R3:
			button_idx = 10;
			break;
		case BUTTON_BLACK:
			button_idx = 16;
			break;
		case BUTTON_WHITE:
			button_idx = 17;
			break;
		case BUTTON_Z:
			button_idx = 18;
			break;
		default:
			printf("UNKNOWN BUTTON: 0x%08x\n", button);
			break;
		}

		mappingarray[i].button = button_idx;
		mappingarray[i].mapping = mapping;
	}

	//printf("DONE!!\n");
}


void registerPS2ControlPatch() {
	usingPS2Controls = 1;
}

void setMenuControls(uint8_t value) {
	if (value <= 2) {
		menuControls = value;

		switch (menuControls) {
		case 0:
			printf("Using Xbox/PC Menu Controls\n");
			break;
		case 1:
			printf("Using 360/Xenon Menu Controls\n");
			patchByte(addr_metabuttonmap + 6, 3);
			break;
		case 2:
			printf("Using PS2 Menu Controls\n");
			patchByte(addr_metabuttonmap + 6, 0);
			break;
		}
	} else {
		printf("Invalid menu controls setting: %d!\n", value);
	}
}

struct cfunclistentry {
	char *name;
	void *func;
};

uint8_t wrap_cfunc(char *name, void *wrapper, void **addr_out) {
	int (*getCfuncCount)() = addr_cfunccount;

	int count = getCfuncCount();
	struct cfunclistentry *cfunclist = addr_cfunclist;

	for (int i = 0; i < count; i++) {
		if (name && strcmp(cfunclist[i].name, name) == 0) {
			if (addr_out) {
				*addr_out = cfunclist[i].func;
			}

			if (wrapper) {
				patchDWord(&(cfunclist[i].func), wrapper);
			}

			return 1;
		}
	}

	printf("Couldn't find cfunc %s!\n", name);

	return 0;
}

uint8_t install_menu_control_patches() {
	uint8_t result = 1;

	uint8_t *addr_buttonmappings;
	

	result &= wrap_cfunc("SetButtonEventMappings", NULL, &addr_buttonmappings);
	result &= patch_cache_pattern("3b 77 04 73 ?? 6a 02 56 ?? cf", &addr_metabuttonmap);

	if (result) {
		patchCall(addr_buttonmappings + 15, setButtonMappingsWrapper);
	}

	return result;
}

uint8_t install_cfunc_patches() {
	uint8_t result = 1;

	result &= wrap_cfunc("FinishPendingZoneLoads", finishloadsWrapper, &addr_finishloads);
	addr_isLoadingLevel = *(uint32_t *)((uint32_t) addr_finishloads + 24);
	//result &= wrap_cfunc("SetScreenElementProps", SetScreenElementPropsWrapper, &addr_setscreenelementprops);	// breaks shops

	patchJmp(addr_isps2, isPs2Wrapper);
	patchJmp(addr_isxenon, isXenonWrapper);
	patchJmp(addr_ispc, isPCWrapper);

	result &= install_menu_control_patches();

	if (!result) {
		printf("FAILED TO FIND CFUNC OFFSETS!\n");
	}

	return result;
}

uint8_t get_script_offsets() {
	uint8_t *parseqbAnchor = NULL;
	uint8_t *crc32Anchor = NULL;
	uint8_t *secondfuncAnchor = NULL;
	uint8_t *collectorseditionAnchor = NULL;
	uint8_t *cfunclistAnchor = NULL;

	uint8_t result = 1;
	result &= patch_cache_pattern("8b 44 24 08 8b 15 ?? ?? ?? ?? 55 56 8b 74 24 0c 8b 4e 04", &addr_unkparseqbfp);
	result &= patch_cache_pattern("8b 44 24 04 55 8b 68 04 56 8d 70 1c 03 e8", &addr_parseqbthirdfunc);
	result &= patch_cache_pattern("6a 01 57 50 e8 ?? ?? ?? ?? 8b 46 10 50", &parseqbAnchor);
	result &= patch_cache_pattern("c1 e1 08 0b ca c1 e1 08 0b c8 51 e8 ?? ?? ?? ?? 83 c4 08", &secondfuncAnchor);
	result &= patch_cache_pattern("a1 ?? ?? ?? ?? 83 f8 01 74 0d 83 f8 02 74 08", &addr_isps2);
	result &= patch_cache_pattern("83 3d ?? ?? ?? ?? 06 0f 94 c0 c3", &addr_isxenon);
	result &= patch_cache_pattern("83 3d ?? ?? ?? ?? 07 0f 94 c0 c3", &addr_ispc);
	result &= patch_cache_pattern("e8 ?? ?? ?? ?? 83 c4 08 84 c0 6a 01 74 07 68 b1 44 be 9d", &collectorseditionAnchor);
	result &= patch_cache_pattern("68 0b 19 50 a7 e8 ?? ?? ?? ?? e8 ?? ?? ?? ?? 50 68 ?? ?? ?? ??", &cfunclistAnchor);

	if (result) {
		addr_parseqb = *(uint32_t *)(parseqbAnchor + 5) + parseqbAnchor + 9;
		their_crc32 = *(uint32_t *)(parseqbAnchor + 14) + parseqbAnchor + 18;
		addr_parseqbsecondfunc = *(uint32_t *)(secondfuncAnchor + 12) + secondfuncAnchor + 16;
		addr_isce = *(uint32_t *)(collectorseditionAnchor + 1) + collectorseditionAnchor + 5;
		addr_cfunclist = *(uint32_t *)(cfunclistAnchor + 17);
		addr_cfunccount = *(uint32_t *)(cfunclistAnchor + 11) + cfunclistAnchor + 15;
	} else {
		printf("FAILED TO FIND SCRIPT OFFSETS\n");
	}

	result &= install_cfunc_patches();

	return result;
}

// TODO: reverse the patching process: check the cached patch list first, then process if there is a patch to be applied.
// an additional nice thing to have would be to cache patches to file (so that they could, say, fall off of a truck)
// a last nice thing would be to load scripts from the local filesystem outside of a cache so that patches can be tested to added to an install

// how do we cache patches correctly?
// check if the patch exists, *then* check the cache.  hopefully we can also access the checksum easily as well.

void __cdecl ParseQbWrapper(char *filename, uint8_t *script, int assertDuplicateSymbols) {
	uint8_t *scriptOut;

	//printf("LOADING SCRIPT %s\n", filename);

	size_t filenameLen = strlen(filename);
	uint8_t *patch = map_get(patchMap, filename, filenameLen);
	if (patch) {
		uint32_t filesize = ((uint32_t *)script)[1];
		scriptCacheEntry *cachedScript = map_get(cachedScriptMap, filename, filenameLen);

		size_t patchLen = map_getsz(patchMap, filename, filenameLen);
		uint32_t patchcrc = getPatchChecksum(patch, patchLen);

		if (cachedScript && cachedScript->checksum == patchcrc) {
			printf("Using cached patched script for %s\n", filename);

			uint8_t *s = map_get(scriptMap, filename, filenameLen);
			if (s) {
				memcpy(s, cachedScript->script, cachedScript->scriptLen);	// rewrite script to memory
			} else {
				s = malloc(cachedScript->scriptLen);
				memcpy(s, cachedScript->script, cachedScript->scriptLen);

				map_put(scriptMap, filename, filenameLen, s, cachedScript->scriptLen);
			}

			scriptOut = s;
		} else {
			if (cachedScript) {
				printf("Cached script checksum %08x didn't match patch %08x\n", cachedScript->checksum, patchcrc);
			}

			printf("Applying patch for %s\n", filename);
			uint8_t *patchedBuffer = NULL;
			size_t patchedBufferLen = 0;

			int result = applyPatch(patch, patchLen, script, filesize, &patchedBuffer, &patchedBufferLen);
				
			if (!result) {
				printf("Patch succeeded! Writing to cache...\n");

				scriptCacheEntry entry;
				entry.filenameLen = filenameLen;
				entry.filename = malloc(filenameLen + 1);
				memcpy(entry.filename, filename, filenameLen);
				entry.filename[filenameLen] = '\0';
				entry.checksum = getPatchChecksum(patch, patchLen);
				entry.scriptLen = patchedBufferLen;
				entry.script = malloc(patchedBufferLen);
				memcpy(entry.script, patchedBuffer, patchedBufferLen);

				map_put(cachedScriptMap, filename, filenameLen, &entry, sizeof(scriptCacheEntry));

				saveScriptCache(scriptCacheFile);

				printf("Done!\n");

				map_put(scriptMap, filename, filenameLen, patchedBuffer, patchedBufferLen);

				scriptOut = patchedBuffer;
			} else {
				scriptOut = script;
				printf("Patch failed! Continuing with original script\n");
			}
		}
	} else {
		scriptOut = script;
	}

	// original function
	uint32_t crc = their_crc32(filename);
	uint32_t idk = addr_parseqbsecondfunc(crc, filename);
	addr_parseqbthirdfunc(scriptOut, scriptOut, addr_unkparseqbfp, crc, idk & 0xffffff00 | (assertDuplicateSymbols != 0));

	script[0] = 1;
}

void patchScriptHook() {
	//printf("patching 0x%08x...\n", addr_parseqb);
	patchCall(addr_parseqb, ParseQbWrapper);
	patchByte(addr_parseqb, 0xe9);	// change CALL to JMP

	patchDWord(0x006d546c, finishloadsWrapper);

	//patchJmp(addr_isce, isCEWrapper);
}

const uint32_t crc32lut[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t crc32(const void *buf, size_t size) {
	const uint8_t *p = buf;
	uint32_t crc;

	crc = 0xffffffff;
	while (size--)
		crc = crc32lut[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
	return crc ^ 0xffffffff;
}


// implementation of Near's beat-protocol (bps) (R.I.P.)

uint8_t readByte(uint8_t *buf, size_t *offset) {
	uint8_t result = buf[*offset];
	(*offset)++;
	return result;
}


uint32_t getPatchChecksum(uint8_t *buf, size_t len) {
	size_t offset = len - 4;
	uint32_t patchcrc = 0;
	patchcrc |= readByte(buf, &offset);
	patchcrc |= readByte(buf, &offset) << 8;
	patchcrc |= readByte(buf, &offset) << 16;
	patchcrc |= readByte(buf, &offset) << 24;
	
	return patchcrc;
}

uint64_t decodeNumber(uint8_t *buf, size_t *offset) {
	uint64_t result = 0;
	uint64_t bitOffset = 0;

	while(1) {
		uint8_t byte = readByte(buf, offset);
		result += (byte & 0x7f) << bitOffset;
		if (byte & 0x80) {
			break;
		}
		bitOffset += 7;
		result += 1 << bitOffset;
	}

	return result;
}

int applyPatch(uint8_t *patch, size_t patchLen, uint8_t *input, size_t inputLen, uint8_t **output, size_t *outputLen) {
	size_t patchOffset = 0;
	size_t outputOffset = 0;

	// header and version ('BPS1') (4 bytes)
	char magicNum[5];
	magicNum[0] = readByte(patch, &patchOffset);
	magicNum[1] = readByte(patch, &patchOffset);
	magicNum[2] = readByte(patch, &patchOffset);
	magicNum[3] = readByte(patch, &patchOffset);
	magicNum[4] = 0;

	if (strcmp(magicNum, "BPS1") != 0) {
		printf("BPS magic number doesn't match expected: %s\n", magicNum);
		return 1;
	}

	// input size (4 bytes, unsigned)
	uint32_t inputExpectedSize = decodeNumber(patch, &patchOffset);
	if (inputExpectedSize != inputLen) {
		printf("Patch input of unexpected size!  expected: %d actual: %d\n", inputExpectedSize, inputLen);
		return 1;
	}

	*outputLen = decodeNumber(patch, &patchOffset);
	*output = malloc(*outputLen);

	uint32_t metadataLen = decodeNumber(patch, &patchOffset);
	patchOffset += metadataLen;	// skip metadata

	int32_t inpOffsetAcc = 0;
	int32_t outOffsetAcc = 0;
	while (patchOffset < patchLen - 12) {
		uint32_t segmentHead = decodeNumber(patch, &patchOffset);
		uint8_t segmentType = segmentHead & 0x03;
		uint32_t segmentLen = (segmentHead >> 2) + 1;
		switch(segmentType) {
			case 0x00: {
				memcpy((*output) + outputOffset, input + outputOffset, segmentLen);
				outputOffset += segmentLen;
				break;
			}
			case 0x01: {
				memcpy((*output) + outputOffset, patch + patchOffset, segmentLen);
				patchOffset += segmentLen;
				outputOffset += segmentLen;
				break;
			}
			case 0x02: {
				int32_t offset = decodeNumber(patch, &patchOffset);
				offset = (offset & 0x01) ? -(offset >> 1) : (offset >> 1);
				inpOffsetAcc += offset;
				memcpy((*output) + outputOffset, input + inpOffsetAcc, segmentLen);
				inpOffsetAcc += segmentLen;
				outputOffset += segmentLen;
				break;
			}
			case 0x03: {
				int32_t offset = decodeNumber(patch, &patchOffset);
				offset = (offset & 0x01) ? -(offset >> 1) : (offset >> 1);
				outOffsetAcc += offset;
				memcpy((*output) + outputOffset, (*output) + outOffsetAcc, segmentLen);
				outOffsetAcc += segmentLen;
				outputOffset += segmentLen;
				break;
			}
		}
	}

	uint32_t inputcrc = 0;
	inputcrc |= readByte(patch, &patchOffset);
	inputcrc |= readByte(patch, &patchOffset) << 8;
	inputcrc |= readByte(patch, &patchOffset) << 16;
	inputcrc |= readByte(patch, &patchOffset) << 24;
	if (inputcrc != crc32(input, inputLen)) {
		printf("INPUT CRC DID NOT MATCH: %x %x\n", inputcrc, crc32(input, inputLen));
		goto failure;
	}

	uint32_t outputcrc = 0;
	outputcrc |= readByte(patch, &patchOffset);
	outputcrc |= readByte(patch, &patchOffset) << 8;
	outputcrc |= readByte(patch, &patchOffset) << 16;
	outputcrc |= readByte(patch, &patchOffset) << 24;
	if (outputcrc != crc32(*output, *outputLen)) {
		printf("OUTPUT CRC DID NOT MATCH: %x %x\n", outputcrc, crc32(*output, *outputLen));
		goto failure;
	}

	uint32_t patchcrc = 0;
	patchcrc |= readByte(patch, &patchOffset);
	patchcrc |= readByte(patch, &patchOffset) << 8;
	patchcrc |= readByte(patch, &patchOffset) << 16;
	patchcrc |= readByte(patch, &patchOffset) << 24;
	if (patchcrc != crc32(patch, patchLen - 4)) {
		printf("PATCH CRC DID NOT MATCH: %x %x\n", patchcrc, crc32(patch, patchLen));
		goto failure;
	}

	return 0;

failure:
	free(*output);
	*output = NULL;
	return 1;
}