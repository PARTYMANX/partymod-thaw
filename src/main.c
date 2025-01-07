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
#define VERSION_NUMBER_MINOR 11
#define VERSION_NUMBER_FIX 3

char configFile[1024];

void *initAddr = NULL;

void *addr_shuffle1;
void *addr_shuffle2;
void *addr_origrand;
uint32_t addr_camera_lookat;
uint32_t addr_aspyr_debug;
uint32_t addr_aspyr_debug_skip_jump;
uint32_t addr_their_PlayMovie;

uint8_t get_misc_offsets() {
	uint8_t *shuffleAnchor = NULL;

	uint8_t result = 1;
	result &= patch_cache_pattern("53 e8 ?? ?? ?? ?? 8b 15 ?? ?? ?? ?? 52 8b f8 e8 ?? ?? ?? ?? 83 c4 08", &shuffleAnchor);
	result &= patch_cache_pattern("f3 0f 5c cc f3 0f 5c d5 f3 0f 5c de 68", &addr_camera_lookat);
	result &= patch_cache_pattern("?? ?? ?? ?? 00 00 8A 44 24 1B 84 C0", &addr_aspyr_debug_skip_jump);
	result &= patch_cache_pattern("3D 00 08 00 00 89 44 24 20", &addr_aspyr_debug);

	if (result) {
		addr_origrand = *(uint32_t *)(shuffleAnchor + 2) + shuffleAnchor + 6;
		addr_shuffle1 = shuffleAnchor + 1;
		addr_shuffle2 = shuffleAnchor + 15;
	} else {
		printf("FAILED TO FIND MISC OFFSETS\n");
	}

	return result;
}

void findOffsets() {
	printf("finding offsets...\n");

	uint8_t result = 1;
	result &= get_config_offsets();
	result &= get_script_offsets();
	result &= get_input_offsets();
	result &= get_misc_offsets();
	result;

	if (!result) {
		printf("PATCHES FAILED!!! %d\n", result);
		//SDL_Delay(3000);
	}

	printf("done!\n");
}

typedef struct {
	float x;
	float y;
	float z;
} vec3f;

vec3f vec3f_normalize(vec3f v) {
	float len = fabsf(sqrtf((v.x * v.x) + (v.y * v.y) + (v.z * v.z)));

	if (len == 0)
		return (vec3f) {0.0, 0.0, 0.0};

	len = 1.0f / len;

	v.x *= len;
	v.y *= len;
	v.z *= len;

	return v;
}

vec3f vec3f_cross(vec3f a, vec3f b) {
	vec3f result;

	result.x = (a.y * b.z) - (a.z * b.y);
	result.y = (a.z * b.x) - (a.x * b.z);
	result.z = (a.x * b.y) - (a.y * b.x);

	return result;
}

float vec3f_dot(vec3f a, vec3f b) {
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

void _stdcall fixedcamera(float *mat_out, vec3f *eye, vec3f *forward, vec3f *up) {
	// lookat function that expects a preprepared forward vector
	// the original function was taking the forward vector, adding the camera position, and going back again which ruined the camera's angle's precision
	// this makes the game move much more smoothly in far edges of the world, like oil rig

	vec3f f = *forward;	//f - front
	vec3f r = vec3f_normalize(vec3f_cross(*up, f));	//r - right
	vec3f u = vec3f_cross(f, r);	//u - up

	// output is in column-major order
	mat_out[0] = r.x;
	mat_out[4] = r.y;
	mat_out[8] = r.z;
	mat_out[12] = -vec3f_dot(r, *eye);
	mat_out[1] = u.x;
	mat_out[5] = u.y;
	mat_out[9] = u.z;
	mat_out[13] = -vec3f_dot(u, *eye);
	mat_out[2] = f.x;
	mat_out[6] = f.y;
	mat_out[10] = f.z;
	mat_out[14] = -vec3f_dot(f, *eye);
	mat_out[3] = 0.0f;
	mat_out[7] = 0.0f;
	mat_out[11] = 0.0f;
	mat_out[15] = 1.0f;
}

#include <d3d9.h>

void _stdcall depthbiaswrapper(IDirect3DDevice9 *device, uint32_t state, float value) {
	HRESULT (_fastcall *setrenderstate)(IDirect3DDevice9 *device, void *pad, IDirect3DDevice9 *alsodevice, uint32_t state, float value) = (void *)device->lpVtbl->SetRenderState;
	float *origdepthbias = 0x00859168;

	value = value;

	printf("DEPTH BIAS: %f (0x%08x), %f\n", *origdepthbias, *origdepthbias, value);

	D3DRS_FOGCOLOR;
	//D3DRS_FOG

	//IDirect3DDevice9_SetRenderState(device, state, value);
	setrenderstate(device, NULL, device, state, value);
}

void patchCamera() {
	// remove camera math
	patchNop(addr_camera_lookat, 12);

	// pass in plain forward vector
	patchByte(addr_camera_lookat + 49 + 3, 0x64);
	patchByte(addr_camera_lookat + 55 + 3, 0x6c);
	patchByte(addr_camera_lookat + 61 + 3, 0x74);

	patchCall(addr_camera_lookat + 67, fixedcamera);
	
	// depth bias tests
	//patchNop(0x0053808d, 6);
	//patchCall(0x0053808d, depthbiaswrapper);

	// fog tests
	//patchByte(0x00527d88 + 1, 1);
	//patchNop(0x00527d8a, 5);

	//patchNop(0x0052812c, 5);
	//patchNop(0x0053225b, 5);
	//patchNop(0x0059b5e0, 5);

	//patchNop(0x00509244, 5);
}

void our_random(int out_of) {
	// first, call the original random so that we consume a value.  
	// juuust in case someone wants actual 100% identical behavior between partymod and the original game
	void (__cdecl *their_random)(int) = addr_origrand;

	their_random(out_of);

	return rand() % out_of;
}

void patchPlaylistShuffle() {
	patchCall(addr_shuffle1, our_random);
	patchCall(addr_shuffle2, our_random);
}

BOOL PlayMovie_wrapper(uint8_t* pParams, uint8_t* pScript) {

	BOOL(__cdecl * play_movie)(uint8_t*, uint8_t*) = (void*)addr_their_PlayMovie;
	uint32_t scriptcrc = 0;

	if (pScript) {
		scriptcrc = *(uint32_t*)(pScript + 0xd0);
		if (scriptcrc == 0xAA37AEAE /*startup_loading_screen*/)
			return TRUE;
	}
	return play_movie(pParams, pScript);
}

void patchIntroMovies() {
	if (!getIniBool("Miscellaneous", "DisplayIntroMovies", 1, configFile)) {
		//patchJmp((void*)0x0047DC00, PlayMovie_wrapper);
		wrap_cfunc("PlayMovie", PlayMovie_wrapper, &addr_their_PlayMovie);
	}
}

void patchStartupSpeed() {
	// Null out mad.bik texture bugging from Aspyr.
	// This is needless and creates needless overhead.
	patchDWord((void*)(addr_aspyr_debug+1), 1);					// For loop from 2048 to 1.
	patchJmp((void*)addr_aspyr_debug_skip_jump, (void*)(addr_aspyr_debug-5));	// Jump past entire check. Fail.
}

void installPatches() {
	patchWindow();
	patchInput();
	patchPlaylistShuffle();
	patchCamera();
	patchStartupSpeed();
	patchIntroMovies();
	printf("installing script patches\n");
	patchScriptHook();
	printf("done\n");
}

uint32_t rng_seed = 0;

void initPatch() {
	GetModuleFileName(NULL, &executableDirectory, filePathBufLen);

	// find last slash
	char *exe = strrchr(executableDirectory, '\\');
	if (exe) {
		*(exe + 1) = '\0';
	}

	sprintf(configFile, "%s%s", executableDirectory, CONFIG_FILE_NAME);

	int isDebug = getIniBool("Miscellaneous", "Debug", 0, configFile);

	if (isDebug) {
		AllocConsole();

		FILE *fDummy;
		freopen_s(&fDummy, "CONIN$", "r", stdin);
		freopen_s(&fDummy, "CONOUT$", "w", stderr);
		freopen_s(&fDummy, "CONOUT$", "w", stdout);
	}
	printf("PARTYMOD for THAW %d.%d.%d\n", VERSION_NUMBER_MAJOR, VERSION_NUMBER_MINOR, VERSION_NUMBER_FIX);

	printf("DIRECTORY: %s\n", executableDirectory);

	init_patch_cache();

	findOffsets();
	installPatches();

	initScriptPatches();

	// get some source of entropy for the music randomizer
	rng_seed = time(NULL) & 0xffffffff;
	srand(rng_seed);

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
