#include <config.h>

#include <windows.h>

#include <stdio.h>
#include <stdint.h>

#include <patch.h>
#include <input.h>
#include <global.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

//int *resX = 0x00851084;
//int *resY = 0x00851088;
//int *bitDepth = 0x0085108c;

uint8_t *highBandwidth = 0x005b4e75;
uint8_t *shadows = 0x005b4e76;
uint8_t *particles = 0x005b4e77;
uint8_t *animatingTextures = 0x005b4e78;
uint8_t *playIntro = 0x005b4e79;
uint8_t *customSettings = 0x008510b1;
uint8_t *distanceFog = 0x008510b2;
uint8_t *lowDetailModels = 0x008510b3;
uint8_t *frameCap = 0x008510b4;

uint8_t isWindowed = 0;
uint8_t *isFullscreen = 0x00858da7;
HWND *hwnd = 0x008b2194;

float *aspectRatio1 = 0x0058eb14;
float *aspectRatio2 = 0x0058d96c;

uint8_t resbuffer[100000];	// buffer needed for high resolutions

uint8_t borderless;

int resX;
int resY;

typedef struct {
	uint32_t BackBufferWidth;
	uint32_t BackBufferHeight;
	uint32_t BackBufferFormat;
	uint32_t BackBufferCount;
	uint32_t MultiSampleType;
	int32_t MultiSampleQuality;
	uint32_t SwapEffect;
	HWND hDeviceWindow;
	uint8_t Windowed;
	uint8_t EnableAutoDepthStencil;
	uint32_t AutoDepthStencilFormat;
	int32_t Flags;
	uint32_t FullScreen_RefreshRateInHz;
	uint32_t PresentationInterval;
} d3dPresentParams;

//d3dPresentParams presentParams;

SDL_Window *window;

void dumpSettings() {
	printf("RESOLUTION X: %d\n", resX);
	printf("RESOLUTION Y: %d\n", resY);
	//printf("BIT DEPTH: %d\n", *bitDepth);
	printf("HIGH BANDWIDTH: %02x\n", *highBandwidth);
	printf("PLAY INTRO: %02x\n", *playIntro);
	printf("CUSTOM SETTINGS: %02x\n", *customSettings);
	printf("ANIMATING TEXTURES: %02x\n", *animatingTextures);
	printf("SHADOWS: %02x\n", *shadows);
	printf("PARTICLES: %02x\n", *particles);
	printf("DISTANCE FOG: %02x\n", *distanceFog);
	printf("LOW DETAIL MODELS: %02x\n", *lowDetailModels);
	printf("LOCK TO 60HZ: %02x\n", *frameCap);
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
	isWindowed = 1;

	SDL_WindowFlags flags = isWindowed ? SDL_WINDOW_SHOWN : SDL_WINDOW_FULLSCREEN;

	if (borderless && isWindowed) {
		flags |= SDL_WINDOW_BORDERLESS;
	}

	*isFullscreen = !isWindowed;

	enforceMaxResolution();

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

	int *isFocused = 0x008a35d5;
	int *other_isFocused = 0x0072e850;
	*isFocused = 1;

	// patch resolution setting
	patchDWord(0x0053515a + 4, resX);	// 0053536a
	patchDWord(0x0053518a + 4, resY);	// 0053539a
	
	SDL_ShowCursor(0);
}

void patchWindow() {
	// replace the window with an SDL2 window.  this kind of straddles the line between input and config
	//DWORD style = WS_CAPTION | WS_ICONIC | WS_CLIPCHILDREN | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE | WS_BORDER;

	//patchDWord((void *)0x00409d55, style);
	//patchDWord((void *)0x00409df4, style);
	patchCall(0x006b3290, createSDLWindow);	// 006691a0
	patchByte(0x006b3290 + 5, 0xc3);

	patchDWord(0x0050d025 + 1, &resbuffer);	// 0050d195

	//patchNop(0x005350ea, 0x100);	// don't set present params
	//patchNop(0x005350ea, 30);
	//patchNop(0x0053513d, 173);

	//patchNop(0x005340df, 6);
	//patchNop(0x005340ef, 6);	// do NOT overwrite resolution

	//patchNop(0x00535244, 8);	// don't overwrite resolution

	patchNop(0x005352b3, 14);	// don't move window to corner	// 005354c3
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

	//GetPrivateProfileString("Game", "HighBandwidth", "", )
	//*highBandwidth = 1;	// it's 2022, very few people are probably on dial up, and probably generally not gaming
	//*playIntro = getIniBool("Miscellaneous", "PlayIntro", 1, configFile);

	//*animatingTextures = getIniBool("Graphics", "AnimatedTextures", 1, configFile);
	//*particles = getIniBool("Graphics", "Particles", 1, configFile);
	//*shadows = getIniBool("Graphics", "Shadows", 1, configFile);
	//*distanceFog = getIniBool("Graphics", "DistanceFog", 0, configFile);
	//*lowDetailModels = getIniBool("Graphics", "LowDetailModels", 0, configFile);
   
	resX = GetPrivateProfileInt("Graphics", "ResolutionX", 640, configFile);
	resY = GetPrivateProfileInt("Graphics", "ResolutionY", 480, configFile);
	if (!isWindowed) {
		isWindowed = getIniBool("Graphics", "Windowed", 0, configFile);
	}
	borderless = getIniBool("Graphics", "Borderless", 0, configFile);

	//*customSettings = 1;    // not useful
	//*frameCap = 1;  // not useful...maybe
	//*bitDepth = 32;	// forcing this to 32 as it's 2022

	float aspectRatio = ((float)resX) / ((float)resY);
	//patchFloat(0x0058eb14, aspectRatio);
	//patchFloat(0x0058d96c, aspectRatio);

	// shadows - i'm not fully sure how this works, but in theory you can increase shadow resolution here
	//patchFloat(0x00501289 + 4, 100.0f);
	//patchFloat(0x00501291 + 4, 100.0f);
	//patchFloat(0x005012a4 + 4, 128.0f);
	//patchFloat(0x005012ac + 4, 128.0f);
}

//char *keyFmtStr = "menu=%d\0cameraToggle=%d\0cameraSwivelLock=%d\0grind=%d\0grab=%d\0ollie=%d\0kick=%d\0spinLeft=%d\0spinRight=%d\0nollie=%d\0switch=%d\0left=%d\0right=%d\0up=%d\0down=%d\0"

#define KEYBIND_SECTION "Keybinds"
#define CONTROLLER_SECTION "Gamepad"

void loadInputSettings(struct inputsettings *settingsOut) {
	char configFile[1024];
	sprintf(configFile, "%s%s", executableDirectory, CONFIG_FILE_NAME);

	if (settingsOut) {
		settingsOut->isPs2Controls = GetPrivateProfileInt(KEYBIND_SECTION, "UsePS2Controls", 1, configFile);
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

		bindsOut->cameraRight = GetPrivateProfileInt(KEYBIND_SECTION, "CameraRight", SDL_SCANCODE_L, configFile);
		bindsOut->cameraLeft = GetPrivateProfileInt(KEYBIND_SECTION, "CameraLeft", SDL_SCANCODE_J, configFile);
		bindsOut->cameraUp = GetPrivateProfileInt(KEYBIND_SECTION, "CameraUp", SDL_SCANCODE_I, configFile);
		bindsOut->cameraDown = GetPrivateProfileInt(KEYBIND_SECTION, "CameraDown", SDL_SCANCODE_K, configFile);
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

void patchLoadConfig() {
	patchCall((void *)0x0040b9f7, loadSettings);
}