#include <windows.h>

#include <stdio.h>
#include <stdint.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <patch.h>
#include <global.h>
#include <input.h>
#include <config.h>
#include <script.h>

#define VERSION_NUMBER_MAJOR 0
#define VERSION_NUMBER_MINOR 1

/*void ledgeWarpFix() {
	// clamp st(0) to [-1, 1]
	// replaces acos call, so we call that at the end
	__asm {
		ftst
		jl negative
		fld1
		fcom
		fstp st(0)
		jle end
		fstp st(0)
		fld1
		jmp end
	negative:
		fchs
		fld1
		fcom
		fstp st(0)
		fchs
		jle end
		fstp st(0)
		fld1
		fchs
	end:
		
	}

	callFunc(0x00577cd7);
}*/

double ledgeWarpFix(double n) {
	//printf("DOING LEDGE WARP FIX\n");
	//double (__cdecl *orig_acos)(double) = (void *)0x00574ad0;

	__asm {
		sub esp,0x08
		fst qword ptr [esp - 0x08]

		ftst
		jl negative
		fld1
		fcom
		fstp st(0)
		jle end
		fstp st(0)
		fld1
		jmp end
	negative:
		fchs
		fld1
		fcom
		fstp st(0)
		fchs
		jle end
		fstp st(0)
		fld1
		fchs
	end:
		
		add esp,0x08

	}

	callFunc(0x00574ad0);

	//return orig_acos(n);
}

void patchLedgeWarp() {
	patchCall(0x004bc32b, ledgeWarpFix);
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

	//patchResolution();

	//initScriptPatches();

	/*int disableMovies = getIniBool("Miscellaneous", "NoMovie", 0, configFile);
	if (disableMovies) {
		printf("Disabling movies\n");
		patchNoMovie();
	}*/

	printf("Patch Initialized\n");
}

void patchNoLauncher() {
	// prevent the setup utility from starting
	// NOTE: if you need some free bytes, there's a lot to work with here, 0x0040b9da to 0x0040ba02 can be rearranged to free up like 36 bytes.  easily enough for a function call
	// the function to load config is in there too, so that can also be taken care of now
	patchNop((void *)0x0040b9da, 7);    // remove call to run launcher
	patchInst((void *)0x0040b9e1, JMP8);    // change launcher condition jump from JZ to JMP
	patchNop((void *)0x0040b9fc, 12);   // remove call to change registry

	// TODO: rename function to make it clear that this adds patch init
	patchCall((void *)0x0040b9da, &(initPatch));
}

void patchNotCD() {
	// Make "notCD" cfunc return true to enable debug features (mostly boring)
	patchByte((void *)0x00404350, 0xb8);
	patchDWord((void *)(0x00404350 + 1), 0x01);
	patchByte((void *)(0x00404350 + 5), 0xc3);
}

__declspec(dllexport) BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
	// Perform actions based on the reason for calling.
	switch(fdwReason) { 
		case DLL_PROCESS_ATTACH:
			// Initialize once for each new process.
			// Return FALSE to fail DLL load.

			// install patches
			patchCall((void *)(0x00544060), &(initPatch));
			//patchNotCD();
			patchWindow();
			//patchNoLauncher();
			//patchIntroMovie();
			//patchLedgeWarp();
			//patchFriction();
			//patchCullModeFix();
			//patchNop(0x0043f037, 6);
			patchInput();
			//patchLoadConfig();
			//patchScriptHook();

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
