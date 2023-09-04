#include <config.h>

#include <windows.h>

#include <stdio.h>
#include <stdint.h>

#include <patch.h>
#include <input.h>
#include <global.h>
#include <patchcache.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

// there has got to be a better way to write this
uint8_t *isFullscreen = 0x00858da7;
uint32_t *resolution_setting = 0x008b455c;
uint8_t *antialiasing = 0x008b4554;
uint8_t *hq_shadows = 0x008b4555;
uint32_t *distance_clipping = 0x008b455a;
uint32_t *clipping_distance = 0x008b4564;	// int from 1-100
uint8_t *fog = 0x008b4556;
HWND *hwnd = 0x008b2194;
int *isFocused = 0x008a35d5;
int *other_isFocused = 0x0072e850;
float *screenAspectRatio = 0x007d0a40;
float *orig_screenanglefactor = NULL;
uint8_t *addr_resX = NULL;
uint8_t *addr_resY = NULL;
uint8_t *addr_createwindow = NULL;
uint8_t *addr_resbuffer = NULL;
uint8_t *addr_cornerwindow = NULL;
uint8_t *addr_loadconfig = NULL;
uint8_t *addr_setaspectratio = NULL;
uint8_t *addr_getscreenangfactor = NULL;
uint8_t *addr_setletterbox = NULL;
uint8_t *isLetterboxed = NULL;
float *viewportYMult = NULL;
float *viewportYOffset = NULL;

uint8_t resbuffer[100000];	// buffer needed for high resolutions

uint8_t isWindowed = 0;
uint8_t borderless;

typedef struct {
	uint32_t antialiasing;
	uint32_t hq_shadows;
	uint32_t distance_clipping;
	uint32_t clipping_distance;	// int from 1-100
	uint32_t fog;
} graphicsSettings;

int resX;
int resY;
float aspect_ratio;
graphicsSettings graphics_settings;

SDL_Window *window;

uint8_t get_config_offsets() {
	uint8_t *isFullscreenAnchor = NULL;
	uint8_t *resolutionSettingAnchor = NULL;
	uint8_t *settingsAnchor = NULL;
	uint8_t *clipdistAnchor = NULL;
	uint8_t *hwndAnchor = NULL;
	uint8_t *isFocusedAnchor = NULL;
	uint8_t *aspectRatioAnchor = NULL;
	uint8_t *setAspectRatioAnchor = NULL;
	uint8_t *getScreenAngleFactorAnchor = NULL;
	uint8_t *letterboxSizeAnchor = NULL;

	uint8_t result = 1;
	result &= patch_cache_pattern("89 44 24 68 a0 ?? ?? ?? ?? 53 33 db", &isFullscreenAnchor);
	result &= patch_cache_pattern("8b 3d ?? ?? ?? ?? 33 c9 84 c0 0f 94 c1", &resolutionSettingAnchor);
	result &= patch_cache_pattern("a8 01 74 07 c6 05 ?? ?? ?? ?? 01 a8 02", &settingsAnchor);
	result &= patch_cache_pattern("a3 ?? ?? ?? ?? 8d 44 24 14 50 8b 44 24 20 8d 4c 24 1c", &clipdistAnchor);
	result &= patch_cache_pattern("75 11 6a 01 8b 15 ?? ?? ?? ?? 52 ff 15", &hwndAnchor);
	result &= patch_cache_pattern("0f 95 c0 84 c0 a2 ?? ?? ?? ?? 0f 85", &isFocusedAnchor);
	result &= patch_cache_pattern("f3 0f 11 44 24 04 e8 ?? ?? ?? ?? d9 05", &aspectRatioAnchor);
	result &= patch_cache_pattern("c7 44 24 2c 80 02 00 00", &addr_resX);
	result &= patch_cache_pattern("c7 44 24 3c e0 01 00 00", &addr_resY);
	result &= patch_cache_pattern("83 ec 40 8b 54 24 44 53 55 56 33 c0", &addr_createwindow);
	result &= patch_cache_pattern("b9 ?? ?? ?? ?? 89 74 24 14 8b ff 8b c3", &addr_resbuffer);
	result &= patch_cache_pattern("6a 04 51 52 53 53 53 50 ff 15", &addr_cornerwindow);
	result &= patch_cache_pattern("e8 ?? ?? ?? ?? 8b 44 24 0c 83 c4 04 6a 00", &addr_loadconfig);
	result &= patch_cache_pattern("e8 ?? ?? ?? ?? 6a 00 68 05 92 52 99 e8 ?? ?? ?? ?? d9 5c 24 24", &setAspectRatioAnchor);
	result &= patch_cache_pattern("e8 ?? ?? ?? ?? d9 44 24 04 8b ce d9 f2 dd d8", &getScreenAngleFactorAnchor);	// three possibilities here, all safe
	result &= patch_cache_pattern("8a 44 24 04 84 c0 a0 ?? ?? ?? ?? 74 23 84 c0", &addr_setletterbox);
	result &= patch_cache_pattern("d1 f8 03 d0 89 15 ?? ?? ?? ?? f3 0f 11 05 ?? ?? ?? ?? c3", &letterboxSizeAnchor);

	if (result) {
		isFullscreen = *(uint32_t *)(isFullscreenAnchor + 5);
		resolution_setting = *(uint32_t *)(resolutionSettingAnchor + 2);
		fog = *(uint32_t *)(settingsAnchor + 6);
		hq_shadows = *(uint32_t *)(settingsAnchor + 6 + (11 * 1));
		antialiasing = *(uint32_t *)(settingsAnchor + 6 + (11 * 5));
		distance_clipping = *(uint32_t *)(settingsAnchor + 6 + (11 * 6) + 1);
		clipping_distance = *(uint32_t *)(clipdistAnchor + 1);
		hwnd = *(uint32_t *)(hwndAnchor + 6);
		isFocused = *(uint32_t *)(isFocusedAnchor + 6);
		aspectRatioAnchor += *(uint32_t *)(aspectRatioAnchor + 7) + 6 + 5;	// follow call
		screenAspectRatio = *(uint32_t *)(aspectRatioAnchor + 2);
		addr_setaspectratio = *(uint32_t *)(setAspectRatioAnchor + 1) + setAspectRatioAnchor + 5;	// should be 0x004ed8b0
		addr_getscreenangfactor = *(uint32_t *)(getScreenAngleFactorAnchor + 1) + getScreenAngleFactorAnchor + 5;
		orig_screenanglefactor = *(uint32_t *)(addr_getscreenangfactor + 2);
		isLetterboxed = *(uint32_t *)(addr_setletterbox + 7);
		viewportYOffset = *(uint32_t *)(letterboxSizeAnchor + 6);
		viewportYMult = *(uint32_t *)(letterboxSizeAnchor + 14);
	} else {
		printf("FAILED TO FIND CONFIG OFFSETS\n");
	}

	return result;
}

void dumpSettings() {
	printf("RESOLUTION X: %d\n", resX);
	printf("RESOLUTION Y: %d\n", resY);
}

void enforceMaxResolution() {
	DEVMODE deviceMode;
	int i = 0;
	uint8_t isValidX = 0;
	uint8_t isValidY = 0;

	while (EnumDisplaySettings(NULL, i, &deviceMode)) {
		if (deviceMode.dmPelsWidth >= resX) {
			isValidX = 1;
		}
		if (deviceMode.dmPelsHeight >= resY) {
			isValidY = 1;
		}

		i++;
	}

	if (!isValidX || !isValidY) {
		resX = 0;
		resY = 0;
	}
}

void loadSettings();

void createSDLWindow() {
	loadSettings();

	SDL_Init(SDL_INIT_VIDEO);
	//isWindowed = 1;

	SDL_WindowFlags flags = isWindowed ? SDL_WINDOW_SHOWN : SDL_WINDOW_FULLSCREEN;

	if (borderless && isWindowed) {
		flags |= SDL_WINDOW_BORDERLESS;
	}

	*isFullscreen = !isWindowed;

	enforceMaxResolution();

	resolution_setting = 0;

	if (resX == 0 || resY == 0) {
		SDL_DisplayMode displayMode;
		SDL_GetDesktopDisplayMode(0, &displayMode);
		resX = displayMode.w;
		resY = displayMode.h;
	}
		
	if (resX < 640) {
		resX = 640;
	}
	if (resY < 480) {
		resY = 480;
	}

	window = SDL_CreateWindow("THAW - PARTYMOD", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, resX, resY, flags);   // TODO: fullscreen

	if (!window) {
		printf("Failed to create window! Error: %s\n", SDL_GetError());
	}

	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);
	*hwnd = wmInfo.info.win.window;

	*isFocused = 1;

	// patch resolution setting
	patchDWord(addr_resX + 4, resX);
	patchDWord(addr_resY + 4, resY);
	
	SDL_ShowCursor(0);
}

void writeConfigValues() {
	*antialiasing = graphics_settings.antialiasing;
	*hq_shadows = graphics_settings.hq_shadows;
	*distance_clipping = graphics_settings.distance_clipping;

	uint32_t distance = graphics_settings.clipping_distance;
	if (distance > 100) {
		distance = 100;
	} else if (distance < 1) {
		distance = 1;
	}
	*clipping_distance = distance * 5.0f + 95.0f;

	*fog = graphics_settings.fog;
}

float __cdecl getScreenAngleFactor() {


	if (*orig_screenanglefactor == 0.0f) {
		printf("waht\n");
	}

	float aspect = ((float)resX / (float)resY);

	float result = ((aspect / (4.0f / 3.0f)));

	//printf("angle: 0x%08x, 0x%08x, %f %f, %d, x%f, off %d\n", addr_getscreenangfactor + 2, orig_screenanglefactor, *orig_screenanglefactor, result, *isLetterboxed, *viewportYMult, *viewportYOffset);
	//*isLetterboxed = 1;
	//*viewportYMult = 1.0f;
	//*viewportYOffset = 2;

	if (*orig_screenanglefactor != 1.0f)
		return *orig_screenanglefactor;
	else 
		return result;
}

void __cdecl setLetterBox(uint8_t isLetterbox) {
	
}

void __cdecl setAspectRatio(float aspect) {
	*screenAspectRatio = (float)resX / (float)resY;
}

void patchWindow() {
	// replace the window with an SDL2 window.  this kind of straddles the line between input and config
	patchCall(addr_createwindow, createSDLWindow);
	patchByte(addr_createwindow + 5, 0xc3);
	
	patchDWord(addr_resbuffer + 1, &resbuffer);

	patchNop(addr_cornerwindow, 14);	// don't move window to corner

	patchCall(addr_loadconfig, writeConfigValues);	// don't load config, use our own

	patchCall(addr_setaspectratio, setAspectRatio);
	patchByte(addr_setaspectratio, 0xe9);	// change CALL to JMP

	patchCall(addr_getscreenangfactor, getScreenAngleFactor);
	patchByte(addr_getscreenangfactor, 0xe9);	// change CALL to JMP
}

#define GRAPHICS_SECTION "Graphics"

int getIniBool(char *section, char *key, int def, char *file) {
	int result = GetPrivateProfileInt(section, key, def, file);
	if (result) {
		return 1;
	} else {
		return 0;
	}
}

void loadSettings() {
	printf("Loading settings\n");

	char configFile[1024];
	sprintf(configFile, "%s%s", executableDirectory, CONFIG_FILE_NAME);

	graphics_settings.antialiasing = getIniBool("Graphics", "AntiAliasing", 0, configFile);
	graphics_settings.hq_shadows = getIniBool("Graphics", "HQShadows", 0, configFile);
	graphics_settings.distance_clipping = getIniBool("Graphics", "DistanceClipping", 0, configFile);
	graphics_settings.clipping_distance = GetPrivateProfileInt("Graphics", "ClippingDistance", 100, configFile);
	graphics_settings.fog = getIniBool("Graphics", "Fog", 0, configFile);
   
	resX = GetPrivateProfileInt("Graphics", "ResolutionX", 0, configFile);
	resY = GetPrivateProfileInt("Graphics", "ResolutionY", 0, configFile);
	if (!isWindowed) {
		isWindowed = getIniBool("Graphics", "Windowed", 0, configFile);
	}
	borderless = getIniBool("Graphics", "Borderless", 0, configFile);
}

#define KEYBIND_SECTION "Keybinds"
#define CONTROLLER_SECTION "Gamepad"

void loadInputSettings(struct inputsettings *settingsOut) {
	char configFile[1024];
	sprintf(configFile, "%s%s", executableDirectory, CONFIG_FILE_NAME);

	if (settingsOut) {
		settingsOut->isPs2Controls = getIniBool("Miscellaneous", "UsePS2Controls", 1, configFile);
	}
}

void loadKeyBinds(struct keybinds *bindsOut) {
	char configFile[1024];
	sprintf(configFile, "%s%s", executableDirectory, CONFIG_FILE_NAME);

	if (bindsOut) {
		bindsOut->menu = GetPrivateProfileInt(KEYBIND_SECTION, "Pause", SDL_SCANCODE_RETURN, configFile);
		bindsOut->cameraToggle = GetPrivateProfileInt(KEYBIND_SECTION, "ViewToggle", SDL_SCANCODE_F, configFile);
		bindsOut->cameraSwivelLock = GetPrivateProfileInt(KEYBIND_SECTION, "SwivelLock", SDL_SCANCODE_GRAVE, configFile);
		bindsOut->focus = GetPrivateProfileInt(CONTROLLER_SECTION, "Focus", SDL_SCANCODE_KP_ENTER, configFile);
		bindsOut->caveman = GetPrivateProfileInt(CONTROLLER_SECTION, "Caveman", SDL_SCANCODE_E, configFile);

		bindsOut->grind = GetPrivateProfileInt(KEYBIND_SECTION, "Grind", SDL_SCANCODE_KP_8, configFile);
		bindsOut->grab = GetPrivateProfileInt(KEYBIND_SECTION, "Grab", SDL_SCANCODE_KP_6, configFile);
		bindsOut->ollie = GetPrivateProfileInt(KEYBIND_SECTION, "Ollie", SDL_SCANCODE_KP_2, configFile);
		bindsOut->kick = GetPrivateProfileInt(KEYBIND_SECTION, "Flip", SDL_SCANCODE_KP_4, configFile);

		bindsOut->leftSpin = GetPrivateProfileInt(KEYBIND_SECTION, "SpinLeft", SDL_SCANCODE_KP_1, configFile);
		bindsOut->rightSpin = GetPrivateProfileInt(KEYBIND_SECTION, "SpinRight", SDL_SCANCODE_KP_3, configFile);
		bindsOut->nollie = GetPrivateProfileInt(KEYBIND_SECTION, "Nollie", SDL_SCANCODE_KP_7, configFile);
		bindsOut->switchRevert = GetPrivateProfileInt(KEYBIND_SECTION, "Switch", SDL_SCANCODE_KP_9, configFile);

		bindsOut->right = GetPrivateProfileInt(KEYBIND_SECTION, "Right", SDL_SCANCODE_D, configFile);
		bindsOut->left = GetPrivateProfileInt(KEYBIND_SECTION, "Left", SDL_SCANCODE_A, configFile);
		bindsOut->up = GetPrivateProfileInt(KEYBIND_SECTION, "Forward", SDL_SCANCODE_W, configFile);
		bindsOut->down = GetPrivateProfileInt(KEYBIND_SECTION, "Backward", SDL_SCANCODE_S, configFile);
		bindsOut->feeble = GetPrivateProfileInt(KEYBIND_SECTION, "Feeble", SDL_SCANCODE_LSHIFT, configFile);

		bindsOut->cameraRight = GetPrivateProfileInt(KEYBIND_SECTION, "CameraRight", SDL_SCANCODE_KP_MULTIPLY, configFile);
		bindsOut->cameraLeft = GetPrivateProfileInt(KEYBIND_SECTION, "CameraLeft", SDL_SCANCODE_KP_DIVIDE, configFile);
		bindsOut->cameraUp = GetPrivateProfileInt(KEYBIND_SECTION, "CameraUp", SDL_SCANCODE_KP_PLUS, configFile);
		bindsOut->cameraDown = GetPrivateProfileInt(KEYBIND_SECTION, "CameraDown", SDL_SCANCODE_KP_MINUS, configFile);

		bindsOut->itemRight = GetPrivateProfileInt(KEYBIND_SECTION, "ItemRight", SDL_SCANCODE_L, configFile);
		bindsOut->itemLeft = GetPrivateProfileInt(KEYBIND_SECTION, "ItemLeft", SDL_SCANCODE_J, configFile);
		bindsOut->itemUp = GetPrivateProfileInt(KEYBIND_SECTION, "ItemUp", SDL_SCANCODE_I, configFile);
		bindsOut->itemDown = GetPrivateProfileInt(KEYBIND_SECTION, "ItemDown", SDL_SCANCODE_K, configFile);
	}
}

void loadControllerBinds(struct controllerbinds *bindsOut) {
	char configFile[1024];
	sprintf(configFile, "%s%s", executableDirectory, CONFIG_FILE_NAME);

	if (bindsOut) {
		bindsOut->menu = GetPrivateProfileInt(CONTROLLER_SECTION, "Pause", CONTROLLER_BUTTON_START, configFile);
		bindsOut->cameraToggle = GetPrivateProfileInt(CONTROLLER_SECTION, "ViewToggle", CONTROLLER_BUTTON_BACK, configFile);
		bindsOut->cameraSwivelLock = GetPrivateProfileInt(CONTROLLER_SECTION, "SwivelLock", CONTROLLER_BUTTON_RIGHTSTICK, configFile);
		bindsOut->focus = GetPrivateProfileInt(CONTROLLER_SECTION, "Focus", CONTROLLER_BUTTON_LEFTSTICK, configFile);
		bindsOut->caveman = GetPrivateProfileInt(CONTROLLER_SECTION, "Caveman", 0, configFile);

		bindsOut->grind = GetPrivateProfileInt(CONTROLLER_SECTION, "Grind", CONTROLLER_BUTTON_Y, configFile);
		bindsOut->grab = GetPrivateProfileInt(CONTROLLER_SECTION, "Grab", CONTROLLER_BUTTON_B, configFile);
		bindsOut->ollie = GetPrivateProfileInt(CONTROLLER_SECTION, "Ollie", CONTROLLER_BUTTON_A, configFile);
		bindsOut->kick = GetPrivateProfileInt(CONTROLLER_SECTION, "Flip", CONTROLLER_BUTTON_X, configFile);

		bindsOut->leftSpin = GetPrivateProfileInt(CONTROLLER_SECTION, "SpinLeft", CONTROLLER_BUTTON_LEFTSHOULDER, configFile);
		bindsOut->rightSpin = GetPrivateProfileInt(CONTROLLER_SECTION, "SpinRight", CONTROLLER_BUTTON_RIGHTSHOULDER, configFile);
		bindsOut->nollie = GetPrivateProfileInt(CONTROLLER_SECTION, "Nollie", CONTROLLER_BUTTON_LEFTTRIGGER, configFile);
		bindsOut->switchRevert = GetPrivateProfileInt(CONTROLLER_SECTION, "Switch", CONTROLLER_BUTTON_RIGHTTRIGGER, configFile);

		bindsOut->right = GetPrivateProfileInt(CONTROLLER_SECTION, "Right", CONTROLLER_BUTTON_DPAD_RIGHT, configFile);
		bindsOut->left = GetPrivateProfileInt(CONTROLLER_SECTION, "Left", CONTROLLER_BUTTON_DPAD_LEFT, configFile);
		bindsOut->up = GetPrivateProfileInt(CONTROLLER_SECTION, "Forward", CONTROLLER_BUTTON_DPAD_UP, configFile);
		bindsOut->down = GetPrivateProfileInt(CONTROLLER_SECTION, "Backward", CONTROLLER_BUTTON_DPAD_DOWN, configFile);

		bindsOut->movement = GetPrivateProfileInt(CONTROLLER_SECTION, "MovementStick", CONTROLLER_STICK_LEFT, configFile);
		bindsOut->camera = GetPrivateProfileInt(CONTROLLER_SECTION, "CameraStick", CONTROLLER_STICK_RIGHT, configFile);
	}
}
