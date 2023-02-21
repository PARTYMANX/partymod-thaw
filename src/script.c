#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <incbin/incbin.h>
#include <Windows.h>

#include <global.h>
#include <patch.h>

#include <hash.h>

// idea for how script patches will work: 
// we hook parseQB which takes the filename and qb contents (sadly, not size)
// if a filename matches a target, we allocate a new struct if it doesn't already exist (probably just in a hashmap)
// we calculate the size by implementing skipqb basically (fuck.), then throw that qb into the patcher
// then we simply patch the file, store it in our new buffer and pass it back to parseqb

INCBIN(groundtricks, "patches/groundtricks.bps");

map_t *scriptMap;
map_t *patchMap;

int applyPatch(uint8_t *patch, size_t patchLen, uint8_t *input, size_t inputLen, uint8_t **output, size_t *outputLen);

void registerPatch(char *name, unsigned int sz, char *data) {
	map_put(patchMap, name, strlen(name), data, sz);
	printf("Registered patch for %s\n", name);
}

void initScriptPatches() {
	patchMap = map_alloc(1, NULL, NULL);
	scriptMap = map_alloc(1, NULL, NULL);

	registerPatch("scripts\\game\\skater\\groundtricks.qb.Wpc", ggroundtricksSize, ggroundtricksData);
}

uint32_t (__cdecl *their_crc32)(char *) = (void *)0x00401b00;
uint32_t (__cdecl *unk_0040a8f0)(uint32_t, char *) = (void *)0x0040a8f0;
void (__cdecl *unk_0040a110)(uint8_t *, uint8_t *, void *, uint32_t, int) = (void *)0x0040a110;

void __cdecl ParseQbWrapper(char *filename, uint8_t *script, int assertDuplicateSymbols) {
	// TODO: match with map
	// if it's in the map, get the qb buffer
	// first we have to get its length by iterating through the qb.  which could be a pain in the ass
	// nvm.  it's literally the second dword

	// TODO: slight change: because this isn't done when we're reading the script (and because of streaming), it's smarter to keep a table of our patched scripts as well as potential patches
	uint8_t *scriptOut;

	//printf("LOADING SCRIPT %s\n", filename);

	uint8_t *patch = map_get(patchMap, filename, strlen(filename));
	if (patch) {
		uint32_t filesize = ((uint32_t *)script)[1];
		uint8_t *patchedScript = map_get(scriptMap, filename, strlen(filename));
		if (patchedScript) {
			scriptOut = patchedScript;
		} else {
			printf("Applying patch for %s\n", filename);
			size_t patchLen = map_getsz(patchMap, filename, strlen(filename));
			uint8_t *patchedBuffer = NULL;
			size_t patchedBufferLen = 0;

			int result = applyPatch(patch, patchLen, script, filesize, &patchedBuffer, &patchedBufferLen);
				
			if (!result) {
				map_put(scriptMap, filename, strlen(filename), patchedBuffer, patchedBufferLen);

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
	uint32_t idk = unk_0040a8f0(crc, filename);
	unk_0040a110(scriptOut, scriptOut, ((void *)0x00417170), crc, idk & 0xffffff00 | (assertDuplicateSymbols != 0));

	script[0] = 1;

	/*__asm {
		push esi
		push edi
		mov edi, dword ptr [esp + 0x0c]
		push edi
		call crc_32
		mov esi, eax
		push edi
		push esi
		call unk_0040a8f0
		mov edx, dword ptr [esp + 0x20]
		test edx, edx
		setnz al
		push eax
		push esi
		mov esi, dword ptr [esp + 0x24]
		push 0x00417170
		push esi
		push esi
		call unk_0040a8f0
		add esp, 0x20
		pop edi
		mov byte ptr [esi], 0x01
		pop esi
		ret
	}*/
}

void patchScriptHook() {
	patchCall(0x0040de30, ParseQbWrapper);
	patchByte(0x0040de30, 0xe9);	// change CALL to JMP
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
	}

	uint32_t outputcrc = 0;
	outputcrc |= readByte(patch, &patchOffset);
	outputcrc |= readByte(patch, &patchOffset) << 8;
	outputcrc |= readByte(patch, &patchOffset) << 16;
	outputcrc |= readByte(patch, &patchOffset) << 24;
	if (outputcrc != crc32(*output, *outputLen)) {
		printf("INPUT CRC DID NOT MATCH: %x %x\n", outputcrc, crc32(*output, *outputLen));
	}

	uint32_t patchcrc = 0;
	patchcrc |= readByte(patch, &patchOffset);
	patchcrc |= readByte(patch, &patchOffset) << 8;
	patchcrc |= readByte(patch, &patchOffset) << 16;
	patchcrc |= readByte(patch, &patchOffset) << 24;
	if (patchcrc != crc32(patch, patchLen - 4)) {
		printf("INPUT CRC DID NOT MATCH: %x %x\n", patchcrc, crc32(patch, patchLen));
	}

	return 0;
}