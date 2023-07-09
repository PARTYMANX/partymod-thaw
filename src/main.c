#include <windows.h>

#include <stdio.h>
#include <stdint.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <patch.h>
#include <patchcache.h>
#include <global.h>
#include <input.h>
#include <config.h>
#include <script.h>

#define VERSION_NUMBER_MAJOR 0
#define VERSION_NUMBER_MINOR 10

void *initAddr = NULL;

void findOffsets() {
	printf("finding offsets...\n");

	uint8_t result = 0;
	result |= get_config_offsets();
	result |= get_script_offsets();

	printf("done!\n");
}

void installPatches() {
	patchWindow();
	patchInput();
	printf("installing script patches\n");
	patchScriptHook();
	printf("done\n");
}

void initPatch() {
	GetModuleFileName(NULL, &executableDirectory, filePathBufLen);

	// find last slash
	char *exe = strrchr(executableDirectory, '\\');
	if (exe) {
		*(exe + 1) = '\0';
	}

	char configFile[1024];
	sprintf(configFile, "%s%s", executableDirectory, CONFIG_FILE_NAME);

	int isDebug = getIniBool("Miscellaneous", "Debug", 0, configFile);

	if (isDebug) {
		AllocConsole();

		FILE *fDummy;
		freopen_s(&fDummy, "CONIN$", "r", stdin);
		freopen_s(&fDummy, "CONOUT$", "w", stderr);
		freopen_s(&fDummy, "CONOUT$", "w", stdout);
	}
	printf("PARTYMOD for THAW %d.%d\n", VERSION_NUMBER_MAJOR, VERSION_NUMBER_MINOR);

	printf("DIRECTORY: %s\n", executableDirectory);

	init_patch_cache();

	findOffsets();
	installPatches();

	initScriptPatches();

	printf("Patch Initialized\n");

	printf("BASE ADDR: 0x%08x, LEN: %d\n", base_addr, mod_size);
	printf("FOUND ADDR: 0x%08x\n", initAddr);
}

void patchHwType() {
	patchByte(0x006283a5 + 6, 0x05);
}

void getModuleInfo() {
	void *mod = GetModuleHandle(NULL);
	//mod = 0x400000;

	base_addr = mod;
	void *end_addr = NULL;

	PIMAGE_DOS_HEADER dos_header = mod;
	PIMAGE_NT_HEADERS nt_headers = (uint8_t *)mod + dos_header->e_lfanew;

	for (int i = 0; i < nt_headers->FileHeader.NumberOfSections; i++) {
		PIMAGE_SECTION_HEADER section = (uint8_t *)nt_headers->OptionalHeader.DataDirectory + nt_headers->OptionalHeader.NumberOfRvaAndSizes * sizeof(IMAGE_DATA_DIRECTORY) + i * sizeof(IMAGE_SECTION_HEADER);

		uint32_t section_size;
		if (section->SizeOfRawData != 0) {
			section_size = section->SizeOfRawData;
		} else {
			section_size = section->Misc.VirtualSize;
		}

		if (section->Characteristics & IMAGE_SCN_MEM_EXECUTE) {
			end_addr = (uint8_t *)base_addr + section->VirtualAddress + section_size;
		}

		if ((i == nt_headers->FileHeader.NumberOfSections - 1) && end_addr == NULL) {
			end_addr = (uint32_t)base_addr + section->PointerToRawData + section_size;
		}
	}

	mod_size = (uint32_t)end_addr - (uint32_t)base_addr;
}

//uint8_t testpattern[17] = {0x83, 0xc4, 0x04, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x8b, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x51, 0x6a, 0x00};

void findInitOffset() {
	findPattern("83 c4 04 e8 ?? ?? ?? ?? 8b 0d ?? ?? ?? ?? 51 6a 00", base_addr, mod_size, &initAddr);
	(uint8_t *)initAddr += 3;
}

__declspec(dllexport) BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
	// Perform actions based on the reason for calling.
	switch(fdwReason) { 
		case DLL_PROCESS_ATTACH:
			// Initialize once for each new process.
			// Return FALSE to fail DLL load.

			getModuleInfo();

			findInitOffset();

			// install patches
			patchCall((void *)(initAddr), &(initPatch));

			break;

		case DLL_THREAD_ATTACH:
			// Do thread-specific initialization.
			break;

		case DLL_THREAD_DETACH:
			// Do thread-specific cleanup.
			break;

		case DLL_PROCESS_DETACH:
			// Perform any necessary cleanup.
			break;
	}
	return TRUE;
}
