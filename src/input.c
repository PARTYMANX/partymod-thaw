#include <windows.h>

#include <stdio.h>
#include <stdint.h>

#include <SDL2/SDL.h>

#include <global.h>
#include <patch.h>
#include <config.h>

// Lookup table that translates from SDL Scancodes to Microsoft Virtual Keys
uint8_t vkeyLUT[285] = {
	0x00, 0x00, 0x00, 0x00, 0x41, 0x42, 0x43, 0x44, // 0, 0, 0, 0, A, B, C, D - 8
	0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, // E, F, G, H, I, J, K, L - 16
	0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, // M, N, O, P, Q, R, S, T - 24
	0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x31, 0x32, // U, V, W, X, Y, Z, 1, 2 - 32
	0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, // 3, 4, 5, 6, 7, 8, 9, 0 - 40
	VK_RETURN, VK_ESCAPE, VK_BACK, VK_TAB, VK_SPACE, VK_OEM_MINUS, VK_OEM_PLUS, VK_OEM_4, // RETURN, ESCAPE, BACKSPACE, TAB, SPACE, MINUS, EQUALS, L BRACKET - 48
	VK_OEM_6, VK_OEM_5, 0x00, VK_OEM_1, VK_OEM_7, VK_OEM_3, VK_OEM_COMMA, VK_OEM_PERIOD, // R BRACKET, BACKSLASH, NONUSLASH, SEMICOLON, APOSTROPHE, GRAVE, COMMA, PERIOD - 56
	VK_OEM_2, VK_CAPITAL, VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, // SLASH, CAPSLOCK, F1, F2, F3, F4, F5, F6 - 64
	VK_F7, VK_F8, VK_F9, VK_F10, VK_F11, VK_F12, VK_PRINT, VK_SCROLL, // F7, F8, F9, F10, F11, F12, PRINT SCREEN, SCROLL LOCK - 72
	VK_PAUSE, VK_INSERT, VK_HOME, VK_PRIOR, VK_DELETE, VK_END, VK_NEXT, VK_RIGHT, // PAUSE, INSERT, HOME, PAGEUP, DELETE, END, PAGEDOWN, RIGHT - 80
	VK_LEFT, VK_DOWN, VK_UP, VK_NUMLOCK, VK_DIVIDE, VK_MULTIPLY, VK_SUBTRACT, VK_ADD, // LEFT, DOWN, UP, NUMLOCKCLEAR, KP DIVIDE, KP MULTIPLY, KP MINUS, KP PLUS - 88
	VK_RETURN, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7, // KP ENTER, KP 1, KP 2, KP 3, KP 4, KP 5, KP 6, KP 7 - 96
	VK_NUMPAD8, VK_NUMPAD9, VK_NUMPAD0, 0x00, 0x00, 0x00, 0x00, 0x00, // KP 8, KP 9, KP 0, KP PERIOD, NONUSBACKSLASH, APPLICATION, POWER, KP EQUALS - 104
	VK_F13, VK_F14, VK_F15, VK_F16, VK_F17, VK_F18, VK_F19, VK_F20, // F13, F14, F15, F16, F17, F18, F19, F20 - 112
	VK_F21, VK_F22, VK_F23, VK_F24, VK_EXECUTE, VK_HELP, 0x00, VK_SELECT, // F21, F22, F23, F24, EXECUTE, HELP, MENU, SELECT - 120
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // STOP, AGAIN, UNDO, CUT, COPY, PASTE, FIND, MUTE - 128
	VK_VOLUME_UP, VK_VOLUME_DOWN, 0x00, 0x00, 0x00, VK_OEM_COMMA, 0x00, 0x00, // VOL UP, VOL DOWN, 0, 0, 0, KP COMMA, KP EQUALS AS400, INTERNATIONAL 1 - 136
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // INTERNATIONAL 2, INTERNATIONAL 3, INTERNATIONAL 4, INTERNATIONAL 5, INTERNATIONAL 6, INTERNATIONAL 7, INTERNATIONAL 8, INTERNATIONAL 9 - 144
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LANG 1, LANG 2, LANG 3, LANG 4, LANG 5, LANG 6, LANG 7, LANG 8 - 152
	0x00, 0x00, 0x00, VK_CANCEL, VK_CLEAR, VK_PRIOR, 0x00, VK_SEPARATOR, // LANG 9, ALTERASE, SYSREQ, CANCEL, CLEAR, PRIOR, RETURN2, SEPARATOR - 160
	0x00, 0x00, 0x00, VK_CRSEL, VK_EXSEL, 0x00, 0x00, 0x00, // OUT, OPER, CLEARAGAIN, CRSEL, EXSEL, 0, 0, 0 - 168
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0, 0, 0, 0, 0, 0, 0, 0 - 176
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // KP 00, KP 000, THOUSANDS SEPARATOR, DECIMAL SEPARATOR, CURRENCY UNIT, CURRENCY SUBUNIT, KP L PAREN, KP R PAREN - 184
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // KP L BRACE, KP R BRACE, KP TAB, KP BACKSPACE, KP A, KP B, KP C, KP D - 192
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // KP E, KP F, KP XOR, KP POWER, KP PERCENT, KP LESS, KP GREATER, KP AMPERSAND - 200
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // KP DBL AMPERSAND, KP VERTICAL BAR, KP DBL VERTICAL BAR, KP COLON, KP HASH, KP SPACE, KP AT, KP EXCLAM - 208
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // KP MEM STORE, KP MEM RECALL, KP MEM CLEAR, KP MEM ADD, KP MEM SUBTRACT, KP MEM MULTIPLY, KP MEM DIVIDE, KP PLUS MINUS - 216
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // KP CLEAR, KP CLEAR ENTRY, KP BINARY, KP OCTAL, KP DECIMAL, KP HEXADECIMAL, 0, 0 - 224
	VK_LCONTROL, VK_LSHIFT, VK_LMENU, VK_LWIN, VK_RCONTROL, VK_RSHIFT, VK_RMENU, VK_RWIN, // L CTRL, L SHIFT, L ALT, L GUI, R CTRL, R SHIFT, R ALT, R GUI - 232
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0, 0, 0, 0, 0, 0, 0, 0 - 240
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0, 0, 0, 0, 0, 0, 0, 0 - 248
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0, 0, 0, 0, 0, 0, 0, 0 - 256
	0x00, 0x00, VK_MEDIA_NEXT_TRACK, VK_MEDIA_PREV_TRACK, VK_MEDIA_STOP, VK_MEDIA_PLAY_PAUSE, VK_VOLUME_MUTE, VK_LAUNCH_MEDIA_SELECT, // 0, MODE, AUDIO NEXT, AUDIO PREV, AUDIO STOP, AUDIO PLAY, AUDIO MUTE, MEDIA SELECT
	0x00, 0x00, 0x00, 0x00, VK_BROWSER_SEARCH, VK_BROWSER_HOME, VK_BROWSER_BACK, VK_BROWSER_FORWARD, // WWW, MAIL, CALCULATOR, COMPUTER, AC SEARCH, AC HOME, AC BACK, AC FORWARD
	VK_BROWSER_STOP, VK_BROWSER_REFRESH, VK_BROWSER_FAVORITES, 0x00, 0x00, 0x00, 0x00, 0x00, // AC STOP, AC REFRESH, AC BOOKMARKS, BRIGHTNESS DOWN, BRIGHTNESS UP, DISPLAY SWITCH, KB DILLUM TOGGLE, KB DILLUM DOWN
	0x00, 0x00, VK_SLEEP, VK_LAUNCH_APP1, 0x00 // KB DILLUM UP, EJECT, SLEEP, APP1, 0
};

typedef struct {
	uint32_t vtablePtr;
	uint32_t node;
	uint32_t type;
	uint32_t port;
	// 16
	uint32_t slot;
	uint32_t isValid;
	uint32_t unk_24;
	uint8_t controlData[32];	// PS2 format control data
	uint8_t vibrationData_align[32];
	uint8_t vibrationData_direct[32];
	uint8_t vibrationData_max[32];
	uint8_t vibrationData_oldDirect[32];    // there may be something before this
	// 160
	// 176
	//uint32_t unk4;
	//uint32_t unk4;
	//uint32_t unk4;
	uint32_t unk5;
	// 192
	uint32_t unk6;
	uint32_t actuatorsDisabled;
	uint32_t capabilities;
	uint32_t unk7;
	// 208
	uint32_t num_actuators;
	uint32_t unk8;
	uint32_t state;
	uint32_t test;
	// 224
	uint32_t index;
	uint32_t isPluggedIn;	// +e4
	uint32_t unplugged_counter;
	uint32_t unplugged_retry;

	uint32_t pressed;
	uint32_t start_or_a_pressed;
	//uint16_t unk14;
	//needs start_or_a_pressed
	// 238
} device;

typedef struct {
	uint32_t unk1;
	uint32_t unk2;
	uint32_t unk3;
	uint32_t unk4;
	//16
	uint32_t unk5;
	uint32_t unk6;
	uint32_t unk7;
	HINSTANCE hinstance;
	//32
	HWND hwnd;
	uint32_t dinputInterface;
} manager;

int (__stdcall *menuOnScreen)() = (void *)0x0044a540;

int controllerCount;
int controllerListSize;
SDL_GameController **controllerList;    // does this need to be threadsafe?
struct keybinds keybinds;
struct controllerbinds padbinds;

uint8_t isCursorActive = 1;
uint8_t isUsingKeyboard = 1;

void setUsingKeyboard(uint8_t usingKeyboard) {
	isUsingKeyboard = usingKeyboard;

	if (isUsingKeyboard && isCursorActive) {
		SDL_ShowCursor(SDL_ENABLE);
	} else {
		SDL_ShowCursor(SDL_DISABLE);
	}
}

// this looks messy to have two functions for cursor hiding/showing, but it makes it easier to inject
void setCursorActive() {
	isCursorActive = 1;

	if (isUsingKeyboard && isCursorActive) {
		SDL_ShowCursor(SDL_ENABLE);
	} else {
		SDL_ShowCursor(SDL_DISABLE);
	}
}

void setCursorInactive() {
	isCursorActive = 0;

	if (isUsingKeyboard && isCursorActive) {
		SDL_ShowCursor(SDL_ENABLE);
	} else {
		SDL_ShowCursor(SDL_DISABLE);
	}
}

void addController(int idx) {
	printf("Detected controller \"%s\"\n", SDL_GameControllerNameForIndex(idx));

	SDL_GameController *controller = SDL_GameControllerOpen(idx);

	if (controller) {
		if (controllerCount == controllerListSize) {
			int tmpSize = controllerListSize + 1;
			SDL_GameController **tmp = realloc(controllerList, sizeof(SDL_GameController *) * tmpSize);
			if (!tmp) {
				return; // TODO: log something here or otherwise do something
			}

			controllerListSize = tmpSize;
			controllerList = tmp;
		}

		controllerList[controllerCount] = controller;
		controllerCount++;
	}
}

void removeController(SDL_GameController *controller) {
	printf("Controller \"%s\" disconnected\n", SDL_GameControllerName(controller));

	int i = 0;

	while (i < controllerCount && controllerList[i] != controller) {
		i++;
	}

	if (controllerList[i] == controller) {
		SDL_GameControllerClose(controller);

		for (; i < controllerCount - 1; i++) {
			controllerList[i] = controllerList[i + 1];
		}
		controllerCount--;
	} else {
		printf("Did not find disconnected controller in list\n");
	}

	//Sleep(10000);
}

void initSDLControllers() {
	printf("Initializing Controller Input\n");

	controllerCount = 0;
	controllerListSize = 1;
	controllerList = malloc(sizeof(SDL_GameController *) * controllerListSize);

	for (int i = 0; i < SDL_NumJoysticks(); i++) {
		if (SDL_IsGameController(i)) {
			addController(i);
		}
	}

	// add event filter for newly connected controllers
	//SDL_SetEventFilter(controllerEventFilter, NULL);
}

uint8_t axisAbs(uint8_t val) {
	if (val > 127) {
		// positive, just remove top bit
		return val & 0x7F;
	} else {
		// negative
		return ~val & 0x7f;
	}
}

uint8_t getButton(SDL_GameController *controller, controllerButton button) {
	if (button == CONTROLLER_BUTTON_LEFTTRIGGER) {
		uint8_t pressure = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) >> 7;
		return pressure > 0x80;
	} else if (button == CONTROLLER_BUTTON_RIGHTTRIGGER) {
		uint8_t pressure = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) >> 7;
		return pressure > 0x80;
	} else {
		return SDL_GameControllerGetButton(controller, button);
	}
}

void getStick(SDL_GameController *controller, controllerStick stick, uint8_t *xOut, uint8_t *yOut) {
	if (stick == CONTROLLER_STICK_LEFT) {
		*xOut = (uint8_t)((SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) >> 8) + 128);
		*yOut = (uint8_t)((SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) >> 8) + 128);
	} else if (stick == CONTROLLER_STICK_RIGHT) {
		*xOut = (uint8_t)((SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX) >> 8) + 128);
		*yOut = (uint8_t)((SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY) >> 8) + 128);
	} else {
		*xOut = 0x80;
		*yOut = 0x80;
	}
}

void pollController(device *dev, SDL_GameController *controller) {
	if (SDL_GameControllerGetAttached(controller)) {
		//printf("Polling controller \"%s\"\n", SDL_GameControllerName(controller));

		dev->isValid = 1;
		dev->isPluggedIn = 1;

		// buttons
		if (getButton(controller, padbinds.menu)) {
			dev->controlData[2] |= 0x01 << 3;
		}
		if (getButton(controller, padbinds.cameraToggle)) {
			dev->controlData[2] |= 0x01 << 0;
		}
		if (getButton(controller, padbinds.cameraSwivelLock)) {
			dev->controlData[2] |= 0x01 << 2;
		}
		if (getButton(controller, padbinds.focus)) {
			dev->controlData[2] |= 0x01 << 1;
		}

		if (getButton(controller, padbinds.grind)) {
			dev->controlData[3] |= 0x01 << 4;
			dev->controlData[12] = 0xff;
		}
		if (getButton(controller, padbinds.grab)) {
			dev->controlData[3] |= 0x01 << 5;
			dev->controlData[13] = 0xff;
		}
		if (getButton(controller, padbinds.ollie)) {
			dev->controlData[3] |= 0x01 << 6;
			dev->controlData[14] = 0xff;
		}
		if (getButton(controller, padbinds.kick)) {
			dev->controlData[3] |= 0x01 << 7;
			dev->controlData[15] = 0xff;
		}

		// shoulders
		if (getButton(controller, padbinds.leftSpin)) {
			dev->controlData[3] |= 0x01 << 2;
			dev->controlData[16] = 0xff;
		}

		if (getButton(controller, padbinds.rightSpin)) {
			dev->controlData[3] |= 0x01 << 3;
			dev->controlData[17] = 0xff;
		}

		if (getButton(controller, padbinds.nollie)) {
			dev->controlData[3] |= 0x01 << 0;
			dev->controlData[18] = 0xff;
		}

		if (getButton(controller, padbinds.switchRevert)) {
			dev->controlData[3] |= 0x01 << 1;
			dev->controlData[19] = 0xff;
		}
		
		// d-pad
		if (SDL_GameControllerGetButton(controller, padbinds.up)) {
			dev->controlData[2] |= 0x01 << 4;
			dev->controlData[10] = 0xFF;
		}
		if (SDL_GameControllerGetButton(controller, padbinds.right)) {
			dev->controlData[2] |= 0x01 << 5;
			dev->controlData[8] = 0xFF;
		}
		if (SDL_GameControllerGetButton(controller, padbinds.down)) {
			dev->controlData[2] |= 0x01 << 6;
			dev->controlData[11] = 0xFF;
		}
		if (SDL_GameControllerGetButton(controller, padbinds.left)) {
			dev->controlData[2] |= 0x01 << 7;
			dev->controlData[9] = 0xFF;
		}

		// big hack: bike backwards revert looks for caveman button specifically, so just bind that to the revert button when in ps2 mode as there are no side effects
		if (getButton(controller, padbinds.switchRevert)) {
			dev->controlData[20] = 0xFF;	// caveman button.  a bit of a hack but i don't know what bit(s) the game is looking for and it ultimately doesn't matter as no other button uses these
		}

		// sticks
		getStick(controller, padbinds.camera, &(dev->controlData[4]), &(dev->controlData[5]));
		getStick(controller, padbinds.movement, &(dev->controlData[6]), &(dev->controlData[7]));

		// apply left stick direction to dpad buttons to allow for menu selections with stick
		// this is safe because park editor does not poll the controller and nothing else separates stick and dpad
		// TODO: check if in menu
		/*if (menuOnScreen()) {
				if (dev->controlData[6] < 64) {
				// > 50% left input
				dev->controlData[2] |= 0x01 << 7;
				dev->controlData[9] = 0xFF;
			} else if (dev->controlData[6] > (255 - 64)) {
				// > 50% right input
				dev->controlData[2] |= 0x01 << 5;
				dev->controlData[8] = 0xFF;
			}

			if (dev->controlData[7] < 64) {
				// > 50% up input
				dev->controlData[2] |= 0x01 << 4;
				dev->controlData[10] = 0xFF;
			} else if (dev->controlData[7] > (255 - 64)) {
				// > 50% down input
				dev->controlData[2] |= 0x01 << 6;
				dev->controlData[11] = 0xFF;
			}
		}*/
	}
}

void pollKeyboard(device *dev) {
	dev->isValid = 1;
	dev->isPluggedIn = 1;

	uint8_t *keyboardState = SDL_GetKeyboardState(NULL);

	// buttons
	if (keyboardState[keybinds.menu]) {
		dev->controlData[2] |= 0x01 << 3;
	}
	if (keyboardState[keybinds.cameraToggle]) {
		dev->controlData[2] |= 0x01 << 0;
	}
	if (keyboardState[keybinds.focus]) {	// no control for left stick on keyboard
		dev->controlData[2] |= 0x01 << 1;
	}
	if (keyboardState[keybinds.cameraSwivelLock]) {
		dev->controlData[2] |= 0x01 << 2;
	}

	if (keyboardState[keybinds.grind]) {
		dev->controlData[3] |= 0x01 << 4;
		dev->controlData[12] = 0xff;
	}
	if (keyboardState[keybinds.grab]) {
		dev->controlData[3] |= 0x01 << 5;
		dev->controlData[13] = 0xff;
	}
	if (keyboardState[keybinds.ollie]) {
		dev->controlData[3] |= 0x01 << 6;
		dev->controlData[14] = 0xff;
	}
	if (keyboardState[keybinds.kick]) {
		dev->controlData[3] |= 0x01 << 7;
		dev->controlData[15] = 0xff;
	}

	// shoulders
	if (keyboardState[keybinds.leftSpin]) {
		dev->controlData[3] |= 0x01 << 2;
		dev->controlData[16] = 0xff;
	}
	if (keyboardState[keybinds.rightSpin]) {
		dev->controlData[3] |= 0x01 << 3;
		dev->controlData[17] = 0xff;
	}
	if (keyboardState[keybinds.nollie]) {
		dev->controlData[3] |= 0x01 << 0;
		dev->controlData[18] = 0xff;
	}
	if (keyboardState[keybinds.switchRevert]) {
		dev->controlData[3] |= 0x01 << 1;
		dev->controlData[19] = 0xff;
	}
		
	// d-pad
	if (keyboardState[keybinds.up]) {
		dev->controlData[2] |= 0x01 << 4;
		dev->controlData[10] = 0xFF;
	}
	if (keyboardState[keybinds.right]) {
		dev->controlData[2] |= 0x01 << 5;
		dev->controlData[8] = 0xFF;
	}
	if (keyboardState[keybinds.down]) {
		dev->controlData[2] |= 0x01 << 6;
		dev->controlData[11] = 0xFF;
	}
	if (keyboardState[keybinds.left]) {
		dev->controlData[2] |= 0x01 << 7;
		dev->controlData[9] = 0xFF;
	}

	// sticks - NOTE: because these keys are very rarely used/important, SOCD handling is just to cancel
	// right
	// x
	if (keyboardState[keybinds.cameraLeft] && !keyboardState[keybinds.cameraRight]) {
		dev->controlData[4] = 0;
	}
	if (keyboardState[keybinds.cameraRight] && !keyboardState[keybinds.cameraLeft]) {
		dev->controlData[4] = 255;
	}

	// y
	if (keyboardState[keybinds.cameraUp] && !keyboardState[keybinds.cameraDown]) {
		dev->controlData[5] = 0;
	}
	if (keyboardState[keybinds.cameraDown] && !keyboardState[keybinds.cameraUp]) {
		dev->controlData[5] = 255;
	}

	// left
	// x
	if (keyboardState[keybinds.left] && !keyboardState[keybinds.right]) {
		dev->controlData[6] = 0;
	}
	if (keyboardState[keybinds.right] && !keyboardState[keybinds.left]) {
		dev->controlData[6] = 255;
	}

	// y
	if (keyboardState[keybinds.up] && !keyboardState[keybinds.down]) {
		dev->controlData[7] = 0;
	}
	if (keyboardState[keybinds.down] && !keyboardState[keybinds.up]) {
		dev->controlData[7] = 255;
	}
}

// returns 1 if a text entry prompt is on-screen so that keybinds don't interfere with text entry confirmation/cancellation
uint8_t isKeyboardTyping() {
	uint8_t *keyboard_on_screen = 0x0074fb42;

	return *keyboard_on_screen;
}

void do_key_input(SDL_KeyCode key) {
	void (*key_input)(int32_t key, uint32_t param) = (void *)0x0062b1f0;
	void (*key_input_other)(int32_t key, uint32_t param) = (void *)0x0062b4bc;
	uint8_t *keyboard_on_screen = 0x0074fb42;

	if (!*keyboard_on_screen) {
		return;
	}

	int32_t key_out = 0;
	uint8_t modstate = SDL_GetModState();
	uint8_t shift = SDL_GetModState() & KMOD_SHIFT;
	uint8_t caps = SDL_GetModState() & KMOD_CAPS;

	if (key == SDLK_RETURN) {
		key_out = 0x0d;	// CR
	} else if (key == SDLK_BACKSPACE) {
		key_out = 0x08;	// BS
	} else if (key == SDLK_ESCAPE) {
		key_out = 0x1b;	// ESC
	} else if (key == SDLK_SPACE) {
		key_out = ' ';
	} else if (key >= SDLK_0 && key <= SDLK_9 && !(modstate & KMOD_SHIFT)) {
		key_out = key;
	} else if (key >= SDLK_a && key <= SDLK_z) {
		key_out = key;
		if (modstate & (KMOD_SHIFT | KMOD_CAPS)) {
			key_out -= 0x20;
		}
	} else if (key == SDLK_PERIOD) {
		if (modstate & KMOD_SHIFT) {
			key_out = '>';
		} else {
			key_out = '.';
		}
	} else if (key == SDLK_COMMA) {
		if (modstate & KMOD_SHIFT) {
			key_out = '<';
		} else {
			key_out = ',';
		}
	} else if (key == SDLK_SLASH) {
		if (modstate & KMOD_SHIFT) {
			key_out = '?';
		} else {
			key_out = '/';
		}
	} else if (key == SDLK_SEMICOLON) {
		if (modstate & KMOD_SHIFT) {
			key_out = ':';
		} else {
			key_out = ';';
		}
	} else if (key == SDLK_QUOTE) {
		if (modstate & KMOD_SHIFT) {
			key_out = '\"';
		} else {
			key_out = '\'';
		}
	} else if (key == SDLK_LEFTBRACKET) {
		if (modstate & KMOD_SHIFT) {
			key_out = '{';
		} else {
			key_out = '[';
		}
	} else if (key == SDLK_RIGHTBRACKET) {
		if (modstate & KMOD_SHIFT) {
			key_out = '}';
		} else {
			key_out = ']';
		}
	} else if (key == SDLK_BACKSLASH) {
		if (modstate & KMOD_SHIFT) {
			key_out = '|';
		} else {
			key_out = '\\';
		}
	} else if (key == SDLK_MINUS) {
		if (modstate & KMOD_SHIFT) {
			key_out = '_';
		} else {
			key_out = '-';
		}
	} else if (key == SDLK_EQUALS) {
		if (modstate & KMOD_SHIFT) {
			key_out = '+';
		} else {
			key_out = '=';
		}
	} else if (key == SDLK_BACKQUOTE) {
		if (modstate & KMOD_SHIFT) {
			key_out = '~';
		} else {
			key_out = '`';
		}
	} else if (key == SDLK_1 && modstate & KMOD_SHIFT) {
		key_out = '!';
	} else if (key == SDLK_2 && modstate & KMOD_SHIFT) {
		key_out = '@';
	} else if (key == SDLK_3 && modstate & KMOD_SHIFT) {
		key_out = '#';
	} else if (key == SDLK_4 && modstate & KMOD_SHIFT) {
		key_out = '$';
	} else if (key == SDLK_5 && modstate & KMOD_SHIFT) {
		key_out = '%';
	} else if (key == SDLK_6 && modstate & KMOD_SHIFT) {
		key_out = '^';
	} else if (key == SDLK_7 && modstate & KMOD_SHIFT) {
		key_out = '&';
	} else if (key == SDLK_8 && modstate & KMOD_SHIFT) {
		key_out = '*';
	} else if (key == SDLK_9 && modstate & KMOD_SHIFT) {
		key_out = '(';
	} else if (key == SDLK_0 && modstate & KMOD_SHIFT) {
		key_out = ')';
	} else {
		key_out = -1;
	}

	//printf("TEST!!\n");

	key_input(key_out, 0);
	//key_input_other(key_out, 0);
}

void processEvent(SDL_Event *e) {


	switch (e->type) {
		case SDL_CONTROLLERDEVICEADDED:
			if (SDL_IsGameController(e->cdevice.which)) {
				addController(e->cdevice.which);
			} else {
				printf("Not a game controller: %s\n", SDL_JoystickNameForIndex(e->cdevice.which));
			}
			return;
		case SDL_CONTROLLERDEVICEREMOVED: {
			SDL_GameController *controller = SDL_GameControllerFromInstanceID(e->cdevice.which);
			if (controller) {
				removeController(controller);
			}
			return;
		}
		case SDL_JOYDEVICEADDED:
			printf("Joystick added: %s\n", SDL_JoystickNameForIndex(e->jdevice.which));
			return;
		case SDL_KEYDOWN: 
			//printf("KEY: %s\n", SDL_GetKeyName(e->key.keysym.sym));
			do_key_input(e->key.keysym.sym);
			return;
		/*case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERAXISMOTION:
			setUsingKeyboard(0);
			return;
		case SDL_MOUSEBUTTONDOWN:
			setUsingKeyboard(1);
			if (e->button.button == SDL_BUTTON_LEFT) {
				void (__stdcall *mouseMove)(int16_t, int16_t, int16_t) = (void*)0x00404950;
				mouseMove(0x0000, (int16_t) e->button.x, (int16_t) e->button.y);
			} else if (e->button.button == SDL_BUTTON_RIGHT) {
				void (__stdcall *mouseMove)(int16_t, int16_t, int16_t) = (void*)0x00405010;
				mouseMove(0x0000, (int16_t) e->button.x, (int16_t) e->button.y);
			} else if (e->button.button == SDL_BUTTON_MIDDLE) {
				void (__stdcall *mouseMove)(int16_t, int16_t, int16_t) = (void*)0x00404dc0;
				mouseMove(0x0000, (int16_t) e->button.x, (int16_t) e->button.y);
			}
			return;
		case SDL_MOUSEBUTTONUP:
			setUsingKeyboard(1);
			if (e->button.button == SDL_BUTTON_LEFT) {
				void (__stdcall *mouseMove)(int16_t, int16_t, int16_t) = (void*)0x00404d20;
				mouseMove(0x0000, (int16_t) e->button.x, (int16_t) e->button.y);
			} else if (e->button.button == SDL_BUTTON_RIGHT) {
				void (__stdcall *mouseMove)(int16_t, int16_t, int16_t) = (void*)0x004052d0;
				mouseMove(0x0000, (int16_t) e->button.x, (int16_t) e->button.y);
			} else if (e->button.button == SDL_BUTTON_MIDDLE) {
				void (__stdcall *mouseMove)(int16_t, int16_t, int16_t) = (void*)0x00404f70;
				mouseMove(0x0000, (int16_t) e->button.x, (int16_t) e->button.y);
			}
			return;
		case SDL_MOUSEMOTION: {
			setUsingKeyboard(1);
			void (__stdcall *mouseMove)(int16_t, int16_t, int16_t) = (void*)0x00405370;
			mouseMove(0x0000, (int16_t) e->motion.x, (int16_t) e->motion.y);
			return;
		}
		case SDL_QUIT: {
			void (__cdecl *resetEngine)() = (void *)0x0041ba40;	// script function to exit game
			resetEngine();
			return;
		}
		case SDL_KEYDOWN: 
			setUsingKeyboard(1);
			return;*/
		case SDL_QUIT: {
			int *shouldQuit = 0x008b2198;
			*shouldQuit = 1;
			return;
		}
		case SDL_WINDOWEVENT_FOCUS_GAINED: {
			int *isFocused = 0x008a35d5;
			int *other_isFocused = 0x0072e850;
			*isFocused = 1;
			return;
		}
		case SDL_WINDOWEVENT_FOCUS_LOST: {
			int *isFocused = 0x008a35d5;
			int *other_isFocused = 0x0072e850;
			*isFocused = 0;
			*other_isFocused = 0;
			return;
		}

			
		default:
			return;
	}
}

void __cdecl processController(device *dev) {
	//printf("Processing Controller %d %d %d!\n", dev->index, dev->slot, dev->port);
	//printf("TYPE: %d\n", dev->type);
	//printf("ISPLUGGEDIN: %d\n", dev->isPluggedIn);
	dev->capabilities = 0x0003;
	dev->num_actuators = 2;
	dev->vibrationData_max[0] = 255;
	dev->vibrationData_max[1] = 255;
	dev->state = 2;
	dev->actuatorsDisabled = 0;

	SDL_Event e;
	while(SDL_PollEvent(&e)) {
		processEvent(&e);
		//printf("EVENT!!!\n");
	}

	dev->isValid = 1;
	dev->isPluggedIn = 1;

	dev->controlData[0] = 0;
	dev->controlData[1] = 0x70;

	// buttons bitmap
	// this bitmap may not work how you would expect. each bit is 1 if the button is *up*, not down
	// the original code sets this initial value at 0xff and XOR's each button bit with the map
	// this scheme cannot be continuously composited, so instead we OR each button with the bitmap and bitwise negate it after all controllers are processed
	dev->controlData[2] = 0x00;
	dev->controlData[3] = 0x00;

	// buttons
	dev->controlData[12] = 0;
	dev->controlData[13] = 0;
	dev->controlData[14] = 0;
	dev->controlData[15] = 0;

	// shoulders
	dev->controlData[16] = 0;
	dev->controlData[17] = 0;
	dev->controlData[18] = 0;
	dev->controlData[19] = 0;

	// d-pad
	dev->controlData[8] = 0;
	dev->controlData[9] = 0;
	dev->controlData[10] = 0;
	dev->controlData[11] = 0;

	// sticks
	dev->controlData[4] = 127;
	dev->controlData[5] = 127;
	dev->controlData[6] = 127;
	dev->controlData[7] = 127;

	dev->controlData[20] = 0;

	if (!isKeyboardTyping()) {
		pollKeyboard(dev);
	}

	// TODO: maybe smart selection of active controller?
	if (dev->port == 0) {
		for (int i = 0; i < controllerCount; i++) {
			pollController(dev, controllerList[i]);
		}
	}

	dev->controlData[2] = ~dev->controlData[2];
	dev->controlData[3] = ~dev->controlData[3];

	if (0xFFFF ^ ((dev->controlData[2] << 8 ) | dev->controlData[3])) {
		dev->pressed = 1;
	} else {
		dev->pressed = 0;
	}

	if (~dev->controlData[2] & 0x01 << 3 || ~dev->controlData[3] & 0x01 << 6) {
		dev->start_or_a_pressed = 1;
	} else {
		dev->start_or_a_pressed = 0;
	}
	
	

	//int (__stdcall *isVibrationOn)() = (void *)0x0041f910;
	//int vibe = isVibrationOn();
	//printf("IS VIBRATION ON: %d\n", vibe);	// enable vibration: 0041f970

	//printf("VIBRATION BUFFERS:\n");
	//printf("%08x%08x%08x%08x%08x%08x%08x%08x\n", dev->vibrationData_align[0], dev->vibrationData_align[1], dev->vibrationData_align[2], dev->vibrationData_align[3], dev->vibrationData_align[4], dev->vibrationData_align[5], dev->vibrationData_align[6], dev->vibrationData_align[7]);
	//printf("%08x%08x%08x%08x%08x%08x%08x%08x\n", dev->vibrationData_direct[0], dev->vibrationData_direct[1], dev->vibrationData_direct[2], dev->vibrationData_direct[3], dev->vibrationData_direct[4], dev->vibrationData_direct[5], dev->vibrationData_direct[6], dev->vibrationData_direct[7]);
	//printf("%08x%08x%08x%08x%08x%08x%08x%08x\n", dev->vibrationData_max[0], dev->vibrationData_max[1], dev->vibrationData_max[2], dev->vibrationData_max[3], dev->vibrationData_max[4], dev->vibrationData_max[5], dev->vibrationData_max[6], dev->vibrationData_max[7]);
	//printf("\n");

	uint8_t *unk1 = 0x0074fb42;
	uint8_t *unk2 = 0x00751dc0;
	uint8_t *unk3 = 0x0074fb43;

	*unk2 = 1;
	*unk3 = 0;

	//printf("UNKNOWN VALUES: 0x0074fb42: %d, 0x00751dc0: %d, 0x0074fb43: %d\n", *unk1, *unk2, *unk3);
}

void __cdecl set_actuators(void *empty, uint16_t left, uint16_t right) {
	//printf("SETTING ACTUATORS: %d %d\n", left, right);
	for (int i = 0; i < controllerCount; i++) {
		SDL_JoystickRumble(SDL_GameControllerGetJoystick(controllerList[i]), left, right, 1000);
	}
}

void __cdecl acquireController(device *dev) {
	printf("Acquiring Controller %d %d %d!\n", dev->index, dev->slot, dev->port);
}

void __cdecl unacquireController(device *dev) {
	//printf("Unacquiring Controller %d!\n", dev->index);
}

void __cdecl createController(device *dev) {
	printf("Initializing Controller index: %d, slot: %d, port: %d!\n", dev->index, dev->slot, dev->port);

	
}

void __cdecl releaseController(device *dev) {
	printf("Releasing Controller %d!\n", dev->index);
}

void __cdecl controllerIsPluggedIn(device *dev) {
	return dev->isPluggedIn;
}

int newDevice(manager* m, int index) {
	// we're treating a __thiscall as a __fastcall here, so we use a dummy parameter in the second slot.
	// added an extra parameter as there seems to be something else pushed to the stack when calling this method
	// WARNING: DO NOT TRY COMPILING THIS WITH O2.  IT DOES NOT WORK.  I DO NOT KNOW WHY
	int (__fastcall *newDevice)(manager *, void *, int, void *) = (void *)0x0040dac0;
	return newDevice(m, NULL, index, NULL);
}

void __stdcall initManager() {
	printf("Initializing Manager!\n");
	//manager->hinstance = hinstance;
	//manager->hwnd = hwnd;

	// init sdl here
	SDL_Init(SDL_INIT_GAMECONTROLLER);

	//SDL_SetHint(SDL_HINT_HIDAPI_IGNORE_DEVICES, "0x1ccf/0x0000");
	SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "false");

	char *controllerDbPath[filePathBufLen];
	int result = sprintf_s(controllerDbPath, filePathBufLen,"%s%s", executableDirectory, "gamecontrollerdb.txt");

	if (result) {
		result = SDL_GameControllerAddMappingsFromFile(controllerDbPath);
		if (result) {
			printf("Loaded mappings\n");
		} else {
			printf("Failed to load %s\n", controllerDbPath);
		}
		
	}

	loadKeyBinds(&keybinds);
	loadControllerBinds(&padbinds);

	initSDLControllers();

	//newDevice(manager, 0);

	//callFunc((void *)0x00403bb0);    // not sure what this is, but it needs to be called or the game crashes
}

int __stdcall getVKeyboardState(uint8_t *out) {
	// translates from SDL scancode to windows vkey.  I want to die.
	int keyCount;
	uint8_t *keystate = SDL_GetKeyboardState(&keyCount);

	memset(out, 0, 256);

	for (int i = 0; i < keyCount; i++) {
		if (keystate[i]) {
			int idx = vkeyLUT[i];
			out[idx] = 0x80;
		}
	}

	/*int (*test1)() = (void *)0x00403df0;
	printf("TEST 1: %d\n", test1());
	uint8_t (*test2)() = (void *)0x004090b0;
	printf("TEST 2: %d\n", test2());

	int (*test3)() = (void *)0x00407a10;
	printf("TEST 3: %d\n", test3());*/

	return 1;
}

uint16_t __stdcall getShiftCtrlState(int key) {
	if (key == 0x10 && SDL_GetModState() & KMOD_SHIFT) {
		return 0x8000;
	} else if (key == 0x11 && SDL_GetModState() & KMOD_CTRL) {
		return 0x8000;
	}

	return 0x0000;
}

uint16_t __stdcall getCapsState(int key) {
	if (SDL_GetModState() & KMOD_CAPS) {
		return 0x0001;
	}

	return 0x0000;
}

int processIntroEvent() {
	int result = 0;

	SDL_Event e;
	while(SDL_PollEvent(&e)) {
		switch(e.type) {
			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_KEYDOWN:
			case SDL_MOUSEBUTTONDOWN:
				if (result == 0) {
					result = 1;
				}
				break;
			case SDL_QUIT:
				result = 2;
				break;
		}

		processEvent(&e);
	}

	return result;
}

void stub() {
	printf("STUB!");
}

void __fastcall rolling_friction_wrapper(void *comp) {
	void (__fastcall *rolling_friction)(void *) = (void *)0x005c8ca0;

	int compAddr = (int)comp;
	float friction = *(float *)(compAddr + 0x678);
	void *padAddr = *(void **)(compAddr + 0x1910);

	int paddr = (int) padAddr;

	uint8_t is_on_bike = *(uint8_t *)(compAddr + 0x524);

	//printf("ROLLING FRICTION: %f\n", friction);
	printf("CONTROL PAD ADDR: 0x%08x\n", padAddr);
	//printf("ARE WE ON A BIKE: %d\n", is_on_bike);
	printf("CONTROLS: 0x320: %d, 0x180: %d, 0x1a0: %d, 0x1c0: %d, 0x1e0: %d, 0x200: %d\n", *(uint8_t *)(paddr + 0x320), *(uint8_t *)(paddr + 0x340), *(uint8_t *)(paddr + 0x1a0), *(uint8_t *)(paddr + 0x1c0), *(uint8_t *)(paddr + 0x1e0), *(uint8_t *)(paddr + 0x200));
	//printf("CONTROLS: 0x220: %d, 0x240: %d, 0x260: %d, 0x280: %d, 0x2a0: %d, 0x2c0: %d, 0x2e0: %d, 0x300: %d\n", *(uint8_t *)(paddr + 0x220), *(uint8_t *)(paddr + 0x240), *(uint8_t *)(paddr + 0x260), *(uint8_t *)(paddr + 0x280), *(uint8_t *)(paddr + 0x2a0), *(uint8_t *)(paddr + 0x2c0), *(uint8_t *)(paddr + 0x2e0), *(uint8_t *)(paddr + 0x300));

	rolling_friction(comp);
}
	//patchCall((void *)(0x005d471c), rolling_friction_wrapper);
	//patchByte((void *)(0x005d471c), 0xe9);

#define spine_buttons_asm(SUCCESS, FAIL) __asm {	\
	__asm push eax	\
	__asm push ebx	\
	__asm push ecx	\
	__asm mov ebx, comp	\
	__asm mov ecx, dword ptr [ebx + 0x1910]	\
	__asm mov al, byte ptr [ecx + 0x120]	/* R2 */	\
	__asm test al, al	\
	__asm jne success	\
	/* ignore L2 if on bike */	\
	__asm mov al, byte ptr [ebx + 0x524]	/* is on bike */ \
	__asm test al, al	\
	__asm jne failure	\
	__asm mov al, byte ptr [ecx + 0xc0]	/* L2 */	\
	__asm test al, al	\
	__asm jne success	\
	\
__asm failure:	\
	__asm pop ecx	\
	__asm pop ebx	\
	__asm pop eax	\
	__asm mov esp, ebp	\
	__asm pop ebp	\
	__asm push FAIL	\
	__asm ret 0x08	\
	\
__asm success:	\
	__asm pop ecx	\
	__asm pop ebx	\
	__asm pop eax	\
	__asm mov esp, ebp	\
	__asm pop ebp	\
	__asm push SUCCESS	\
	__asm ret 0x08	\
}

#define not_spine_buttons_asm(SUCCESS, FAIL) __asm {	\
	__asm push eax	\
	__asm push ebx	\
	__asm push ecx	\
	__asm mov ebx, comp	\
	__asm mov ecx, dword ptr [ebx + 0x1910]	\
	__asm mov al, byte ptr [ecx + 0x120]	/* R2 */	\
	__asm test al, al	\
	__asm jne failure	\
	/* ignore L2 if on bike */	\
	__asm mov al, byte ptr [ebx + 0x524]	/* is on bike */ \
	__asm test al, al	\
	__asm jne success	\
	__asm mov al, byte ptr [ecx + 0xc0]	/* L2 */	\
	__asm test al, al	\
	__asm jne failure	\
	\
__asm success:	\
	__asm pop ecx	\
	__asm pop ebx	\
	__asm pop eax	\
	__asm mov esp, ebp	\
	__asm pop ebp	\
	__asm push SUCCESS	\
	__asm ret 0x08	\
	\
__asm failure:	\
	__asm pop ecx	\
	__asm pop ebx	\
	__asm pop eax	\
	__asm mov esp, ebp	\
	__asm pop ebp	\
	__asm push FAIL	\
	__asm ret 0x08	\
}

void __stdcall in_air_to_break(void *comp) {
	spine_buttons_asm(0x005dff5f, 0x005dff55);
}

void break_vert(void *comp) {
	not_spine_buttons_asm(0x005da2f0, 0x005da306);
}

void other_break_vert(void *comp) {
	not_spine_buttons_asm(0x005da31e, 0x005da561);
}

void lip_jump(void *comp) {
	spine_buttons_asm(0x005c9cbd, 0x005c9d41)
}

void air_recovery(void *comp) {
	spine_buttons_asm(0x005df984, 0x005df98b);
}

void ground_to_air(void *comp) {
	not_spine_buttons_asm(0x005dea3c, 0x005dea45);
}

void ground_to_air_acid_drop(void *comp) {
	not_spine_buttons_asm(0x005df02f, 0x005defff);
}

void also_ground_to_air_acid_drop(void *comp) {
	spine_buttons_asm(0x005df3a5, 0x005df7af);
}

void __stdcall in_air_acid_drop(void *comp) {
	not_spine_buttons_asm(0x005e0d19, 0x005e0d23);
}

void walk_acid_drop(void *comp) {
	__asm {
		push eax
		push ecx
		mov eax, comp
		mov ecx, dword ptr [eax + 0x17cc]
		mov al, byte ptr [ecx + 0x120]	/* R2 */
		test al, al
		jne success
		mov al, byte ptr [ecx + 0xc0]	/* L2 */
		test al, al
		jne success

		pop ecx
		pop eax
		mov esp, ebp
		pop ebp
		push 0x005fcdc7	/* false */
		ret 0x08

	success:
		pop ecx
		pop eax
		mov esp, ebp
		pop ebp
		push 0x005fcd0a	/* true */
		ret 0x08
	}
}

void not_bike_lip_check(void *comp) {
	__asm {
		push eax
		push ecx
		mov eax, comp
		mov ecx, dword ptr [eax + 0x1910]
		mov al, byte ptr [ecx + 0x100]	/* R1 */
		test al, al
		jne success
		mov al, byte ptr [ecx + 0xa0]	/* L1 */
		test al, al
		jne success

		pop ecx
		pop eax
		mov esp, ebp
		pop ebp
		push 0x005ce9c8	/* false */
		ret 0x08

	success:
		pop ecx
		pop eax
		mov esp, ebp
		pop ebp
		push 0x005ceb7d	/* true */
		ret 0x08
	}
}

void bike_lip_check(void *comp) {
	__asm {
		push eax
		push ecx
		mov eax, comp
		mov ecx, dword ptr [eax + 0x1910]
		mov al, byte ptr [ecx + 0x100]	/* R1 */
		test al, al
		jne success
		mov al, byte ptr [ecx + 0xa0]	/* L1 */
		test al, al
		jne success

		pop ecx
		pop eax
		mov esp, ebp
		pop ebp
		push 0x005dc446	/* false */
		ret 0x08

	success:
		pop ecx
		pop eax
		mov esp, ebp
		pop ebp
		push 0x005dc327	/* true */
		ret 0x08
	}
}

void patchPs2Buttons() {
	//patchCall((void *)(0x005d471c), rolling_friction_wrapper);
	//patchByte((void *)(0x005d471c), 0xe9);

	patchByte((void *)(0x0046ee86 + 2), 0x05);	// change PC platform to gamecube.  this just makes it default to ps2 controls
	//patchByte((void *)(0x0046ef29 + 2), 0x05);	// do ps2 things if xbox

	// in air
	patchByte((void *)(0x005dff41), 0x56);	// PUSH ESI
	patchCall((void *)(0x005dff41 + 1), in_air_to_break);

	// break vert
	patchByte((void *)(0x005da2dc), 0x56);	// PUSH ESI
	patchCall((void *)(0x005da2dc + 1), break_vert);

	patchByte((void *)(0x005da306), 0x56);	// PUSH ESI
	patchCall((void *)(0x005da306 + 1), other_break_vert);

	// bike lip check
	//patchByte((void *)(0x005da32c + 2), 0x00);
	//patchByte((void *)(0x005da32c + 3), 0x01);

	// lip jump?
	patchByte((void *)(0x005c9ca1), 0x52);	// PUSH EBX
	patchCall((void *)(0x005c9ca1 + 1), lip_jump);

	// check side
	//patchByte((void *)(0x005dad77 + 2), 0xa0);
	//patchByte((void *)(0x005dad85 + 2), 0x00);
	//patchByte((void *)(0x005dad85 + 3), 0x01);

	// air recovery
	patchByte((void *)(0x005df970), 0x56);	// PUSH ESI
	patchCall((void *)(0x005df970 + 1), air_recovery);

	// bike lean/fast turn
	//patchByte((void *)(0x005c8b4a + 2), 0xa0);
	//patchByte((void *)(0x005c8b5d + 2), 0x00);
	//patchByte((void *)(0x005c8b5d + 3), 0x01);

	// ground-to-air
	patchByte((void *)(0x005dea28), 0x56);	// PUSH ESI
	patchCall((void *)(0x005dea28 + 1), ground_to_air);

	// ground-to-air acid drop
	patchByte((void *)(0x005defeb), 0x56);	// PUSH ESI
	patchCall((void *)(0x005defeb + 1), ground_to_air_acid_drop);

	// also ground-to-air acid drop
	patchByte((void *)(0x005df389), 0x56);	// PUSH ESI
	patchCall((void *)(0x005df389 + 1), also_ground_to_air_acid_drop);

	// acid drop
	patchByte((void *)(0x005e0d05), 0x56);	// PUSH ESI
	patchCall((void *)(0x005e0d05 + 1), in_air_acid_drop);

	// walk acid drop
	patchByte((void *)(0x005fcceb), 0x56);	// PUSH ESI
	patchCall((void *)(0x005fcceb + 1), walk_acid_drop);

	// bike lip
	//patchByte((void *)(0x005ce99a + 2), 0x00);
	//patchByte((void *)(0x005ce99a + 3), 0x01);
	patchByte((void *)(0x005ce99a), 0x53);	// PUSH EBX
	patchByte((void *)(0x005ce99a + 1), 0x55);	// PUSH EBP
	patchByte((void *)(0x005ce99a + 2), 0x56);	// PUSH ESI
	patchCall((void *)(0x005ce99a + 3), not_bike_lip_check);
	
	patchByte((void *)(0x005dc301), 0x56);	// PUSH ESI
	patchCall((void *)(0x005dc301 + 1), bike_lip_check);
	// change trick selection to if r1 or l1 is pressed
	patchByte((void *)(0x005dc9be + 2), 0x80);
	patchByte((void *)(0x005dc9be + 3), 0x00);

	// natas spin
	//patchByte((void *)(0x005e0f01 + 2), 0xa0);
	//patchByte((void *)(0x005e0ef7 + 2), 0x00);
	//patchByte((void *)(0x005e0ef7 + 3), 0x01);

	// stall
	patchByte((void *)(0x005e0f8e + 2), 0x00);
	patchByte((void *)(0x005e0f8e + 3), 0x01);

	patchByte((void *)(0x005e2939 + 2), 0x00);
	patchByte((void *)(0x005e2939 + 3), 0x01);

	// bike fast spin/flip
	patchByte((void *)(0x005d7d99 + 2), 0xa0);

	// disable spin delay on l1/r1
	patchNop((void *)(0x005cd230), 2);
	patchNop((void *)(0x005cd2fb), 2);
}

void patchInput() {
	// patch SIO::Device
	// process
	patchThisToCdecl((void *)0x0062b090, &processController);
	patchByte((void *)(0x0062b090 + 7), 0xC3);
	//patchByte((void *)(0x0062b090 + 8), 0x04);
	//patchByte((void *)(0x0062b090 + 9), 0x00);

	//patchThisToCdecl((void *)0x0062a570, &processController);
	//patchByte((void *)(0x0062a570 + 7), 0xC3);

	// acquire
	//patchThisToCdecl((void *)0x00541f70, &acquireController);
	//patchByte((void *)(0x00541f70 + 7), 0xC3);
	// unacquire - NOT FOUND!!
	//patchThisToCdecl((void *)0x0040d060, &unacquireController);
	//patchByte((void *)(0x0040d060 + 7), 0xC3);
	// init
	//patchThisToCdecl((void *)0x00541df0, &initController);
	//patchByte((void *)(0x00541df0 + 7), 0xC2);
	//patchByte((void *)(0x00541df0 + 8), 0x04);
	//patchByte((void *)(0x00541df0 + 9), 0x00);
	// release
	//patchThisToCdecl((void *)0x00541ed0, &releaseController);
	//patchByte((void *)(0x00541ed0 + 7), 0xC3);

	//activateactuator
	//patchByte((void *)(0x0062add0), 0xC2);
	//patchByte((void *)(0x0062add0 + 1), 0x08);
	//patchByte((void *)(0x0062add0 + 2), 0x00);

	// set_actuator
	// don't call read_data in activate_actuators
	patchNop(0x0062ade1, 5);
	patchCall(0x0062ae64, set_actuators);
	patchCall(0x0062aec7, set_actuators);
	patchCall(0x0062af6f, set_actuators);
	patchCall(0x0062b001, set_actuators);
	patchCall(0x0062b05f, set_actuators);
	//patchByte((void *)(0x006b40a0), 0xe9);

	// fix is_plugged_in crash???? TODO: figure out why it was getting an exception when accessing the device
	//patchThisToCdecl((void *)0x0062a4a0, &controllerIsPluggedIn);
	//patchByte((void *)(0x0062a4a0 + 7), 0xc3);
	//patchByte((void *)(0x0062a4a0), 0xb0);
	//patchByte((void *)(0x0062a4a0 + 1), 0x01);
	//patchByte((void *)(0x0062a4a0 + 2), 0xc3);


	// patch SIO::Manager
	// 0x40db20 - Init
	// init
	//patchNop((void *)0x0040db3a, 66);   // directinput8create
	//patchNop((void *)0x0040db20, 156);
	//patchNop((void *)0x0040db81, 4);  // setup newdevice call (needed)
	//patchNop((void *)0x0040db8f, 15); // newdevice (0) (needed)
	//patchNop((void *)0x0040db9e, 18);   // enumdevices
	//patchNop(0x0040dbb0, 5);  // function that must be called
	
	// init input patch - nop direct input setup
	//patchNop(0x005430fc, 49);
	patchNop(0x006b4c23, 45);
	patchCall(0x006b4c23 + 5, &initManager);
	//patchCall(0x006b4c23 + 5, &stub);

	// some config call relating to the dinput devices
	patchNop(0x0054481c, 5);


	patchPs2Buttons();

	//patchNop(0x0054310A, 35);	// DirectInput8Create
	//patchNop(0x0054312d, 8);	// createDevice
	//patchNop(0x00543147, 12);	// createDevice
	//patchNop(0x00543153, 11);	// sleep
	//patchCall(0x006b4c35 + 5, &initManager);

	//patchThisToCdecl((void *)0x0040db20, &initManager);
	// MOV EAX,0x01 (return 1) NOTE: we're only doing this to be good.  nothing ever reads the return value
	//patchInst((void *)(0x00543070 + 7), MOVIMM8);
	//patchByte((void *)(0x00543070 + 8), 0x01);
	// RET 0x0008 (pop twice, return);
	//patchByte((void *)(0x00543070 + 9), 0xC2);
	//patchByte((void *)(0x00543070 + 10), 0x08);
	//patchByte((void *)(0x00543070 + 11), 0x00);
	//patchNop(0x0040db20, 12);  // function that must be called

	// patch keyboard input
	/*patchNop((void *)0x00403c66, 6);
	patchCall((void *)0x00403c66, &getVKeyboardState);

	patchNop((void *)0x00403c74, 6);
	patchCall((void *)0x00403c74, &getShiftCtrlState);

	patchNop((void *)0x00403c85, 6);
	patchCall((void *)0x00403c85, &getCapsState);

	// always say window is active
	patchNop((void *)0x004090b0, 5);
	patchInst((void *)0x004090b0, MOVIMM8);
	patchByte((void *)(0x004090b0 + 1), 0x01);

	//patchByte((void *)0x00403d90, 0x74);

	// park editor call.  this one's weird
	//patchDWord((void *)0x00461608, (uint32_t)getShiftCtrlState);

	// use our own cursor logic
	// show cursor
	patchCall((void *)0x00405780, &setCursorActive);
	patchByte((void *)(0x00405780 + 5), 0xC3);
	// hide cursor
	patchCall((void *)0x004057C0, &setCursorInactive);
	patchByte((void *)(0x004057C0 + 5), 0xC3);*/

}