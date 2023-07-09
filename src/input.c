#include <windows.h>

#include <stdio.h>
#include <stdint.h>

#include <SDL2/SDL.h>

#include <global.h>
#include <patch.h>
#include <config.h>
#include <script.h>

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
	// 238
} device;

void patchPs2Buttons();

int controllerCount;
int controllerListSize;
SDL_GameController **controllerList;
struct inputsettings inputsettings;
struct keybinds keybinds;
struct controllerbinds padbinds;

uint8_t isUsingKeyboard = 1;

struct playerslot {
	SDL_GameController *controller;
	uint8_t lockedOut;	// after sign-in, a controller is "locked out" until all of its buttons are released
};

#define MAX_PLAYERS 2
uint8_t numplayers = 0;
struct playerslot players[MAX_PLAYERS] = { { NULL, 0 }, { NULL, 0 } };

void setUsingKeyboard(uint8_t usingKeyboard) {
	isUsingKeyboard = usingKeyboard;
}

void addController(int idx) {
	printf("Detected controller \"%s\"\n", SDL_GameControllerNameForIndex(idx));

	SDL_GameController *controller = SDL_GameControllerOpen(idx);

	SDL_GameControllerSetPlayerIndex(controller, -1);

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

void addplayer(SDL_GameController *controller) {
	if (numplayers < 2) {
		// find open slot
		uint8_t found = 0;
		int i = 0;
		for (; i < MAX_PLAYERS; i++) {
			if (!players[i].controller) {
				found = 1;
				break;
			}
		}
		if (found) {
			SDL_GameControllerSetPlayerIndex(controller, i);
			players[i].controller = controller;

			if (numplayers > 0) {
				players[i].lockedOut = 1;
				SDL_JoystickRumble(SDL_GameControllerGetJoystick(controller), 0x7fff, 0x7fff, 250);
			}
			
			numplayers++;

			printf("Added player %d: %s\n", i + 1, SDL_GameControllerName(controller));
		}
	} else {
		printf("Already two players, not adding\n");
	}
}

void pruneplayers() {
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (players[i].controller && !SDL_GameControllerGetAttached(players[i].controller)) {
			printf("Pruned player %d\n", i + 1);

			players[i].controller = NULL;
			numplayers--;
			printf("Remaining players: %d\n", numplayers);
		}
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

		int playerIdx = SDL_GameControllerGetPlayerIndex(controller);
		if (playerIdx != -1) {
			printf("Removed player %d\n", playerIdx + 1);
			players[playerIdx].controller = NULL;
			numplayers--;
		}

		pruneplayers();

		for (; i < controllerCount - 1; i++) {
			controllerList[i] = controllerList[i + 1];
		}
		controllerCount--;
	} else {
		//setActiveController(NULL);
		printf("Did not find disconnected controller in list\n");
	}
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
	uint8_t result_x, result_y;

	if (stick == CONTROLLER_STICK_LEFT) {
		result_x = (uint8_t)((SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) >> 8) + 128);
		result_y = (uint8_t)((SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) >> 8) + 128);
	} else if (stick == CONTROLLER_STICK_RIGHT) {
		result_x = (uint8_t)((SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX) >> 8) + 128);
		result_y = (uint8_t)((SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY) >> 8) + 128);
	} else {
		result_x = 0x80;
		result_y = 0x80;
	}

	if (axisAbs(result_x) > axisAbs(*xOut)) {
		*xOut = result_x;
	}

	if (axisAbs(result_y) > axisAbs(*yOut)) {
		*yOut = result_y;
	}
}

void pollController(device *dev, SDL_GameController *controller) {
	if (SDL_GameControllerGetAttached(controller)) {
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

		if (inputsettings.isPs2Controls) {
			// big hack: bike backwards revert looks for white button specifically, so just bind that to the revert button when in ps2 mode as there are no side effects
			if (getButton(controller, padbinds.switchRevert)) {
				dev->controlData[20] |= 0x01 << 1;
			} 
		} else {
			if (getButton(controller, padbinds.nollie)) {
				dev->controlData[20] |= 0x01 << 1;
			}

			// caveman
			if (getButton(controller, padbinds.switchRevert)) {
				dev->controlData[20] |= 0x01 << 0;
			}
		}
		
		// sticks
		getStick(controller, padbinds.camera, &(dev->controlData[4]), &(dev->controlData[5]));
		getStick(controller, padbinds.movement, &(dev->controlData[6]), &(dev->controlData[7]));
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

	if (inputsettings.isPs2Controls) {
		// big hack: bike backwards revert looks for white button specifically, so just bind that to the revert button when in ps2 mode as there are no side effects
		if (keyboardState[keybinds.switchRevert]) {
			dev->controlData[20] |= 0x01 << 1;
		} 
	} else {
		if (keyboardState[keybinds.nollie]) {
			dev->controlData[20] |= 0x01 << 1;
		}

		// caveman
		if (keyboardState[keybinds.switchRevert]) {
			dev->controlData[20] |= 0x01 << 0;
		}
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
	} else if (key == SDLK_KP_0) {
		key_out = '0';
	} else if (key == SDLK_KP_1) {
		key_out = '1';
	} else if (key == SDLK_KP_2) {
		key_out = '2';
	} else if (key == SDLK_KP_3) {
		key_out = '3';
	} else if (key == SDLK_KP_4) {
		key_out = '4';
	} else if (key == SDLK_KP_5) {
		key_out = '5';
	} else if (key == SDLK_KP_6) {
		key_out = '6';
	} else if (key == SDLK_KP_7) {
		key_out = '7';
	} else if (key == SDLK_KP_8) {
		key_out = '8';
	} else if (key == SDLK_KP_9) {
		key_out = '9';
	} else if (key == SDLK_KP_MINUS) {
		key_out = '-';
	} else if (key == SDLK_KP_EQUALS) {
		key_out = '=';
	} else if (key == SDLK_KP_PLUS) {
		key_out = '+';
	} else if (key == SDLK_KP_DIVIDE) {
		key_out = '/';
	} else if (key == SDLK_KP_MULTIPLY) {
		key_out = '*';
	} else if (key == SDLK_KP_DECIMAL) {
		key_out = '.';
	} else if (key == SDLK_KP_ENTER) {
		key_out = 0x0d;
	} else {
		key_out = -1;
	}

	key_input(key_out, 0);
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
			setUsingKeyboard(1);
			do_key_input(e->key.keysym.sym);
			return;
		case SDL_CONTROLLERBUTTONDOWN: {
			SDL_GameController *controller = SDL_GameControllerFromInstanceID(e->cdevice.which);

			int idx = SDL_GameControllerGetPlayerIndex(controller);
			if (idx == -1) {
				addplayer(controller);
			} else if (players[idx].lockedOut) {
				players[idx].lockedOut++;
			}

			setUsingKeyboard(0);
			return;
		}
		case SDL_CONTROLLERBUTTONUP: {
			SDL_GameController *controller = SDL_GameControllerFromInstanceID(e->cdevice.which);

			int idx = SDL_GameControllerGetPlayerIndex(controller);
			if (idx != -1 && players[idx].lockedOut) {
				players[idx].lockedOut--;
			}

			return;
		}
		case SDL_CONTROLLERAXISMOTION:
			setUsingKeyboard(0);
			return;
		case SDL_QUIT: {
			int *shouldQuit = 0x008b2198;	// 0084aa80
			*shouldQuit = 1;
			return;
		}
		default:
			return;
	}
}

void __cdecl processController(device *dev) {
	// cheating:
	// replace type with index
	//dev->type = 60;

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

	dev->isValid = 0;
	dev->isPluggedIn = 0;

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

	if (dev->port == 0) {
		dev->isValid = 1;
		dev->isPluggedIn = 1;

		if (!isKeyboardTyping()) {
			pollKeyboard(dev);
		}
	}
	
	if (dev->port < MAX_PLAYERS) {
		if (players[dev->port].controller && !players[dev->port].lockedOut) {
			pollController(dev, players[dev->port].controller);
		}
	}

	dev->controlData[2] = ~dev->controlData[2];
	dev->controlData[3] = ~dev->controlData[3];
	//dev->controlData[20] = ~dev->controlData[20];

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

	// keyboard text entry doesn't work unless these values are set correctly
	uint8_t *unk2 = 0x00751dc0;
	uint8_t *unk3 = 0x0074fb43;

	*unk2 = 1;
	*unk3 = 0;

	//printf("UNKNOWN VALUES: 0x0074fb42: %d, 0x00751dc0: %d, 0x0074fb43: %d\n", *unk1, *unk2, *unk3);
}

void __cdecl set_actuators(int port, uint16_t left, uint16_t right) {
	//printf("SETTING ACTUATORS: %d %d %d\n", port, left, right);
	for (int i = 0; i < controllerCount; i++) {
		if (SDL_GameControllerGetAttached(controllerList[i]) && SDL_GameControllerGetPlayerIndex(controllerList[i]) == port) {
			SDL_JoystickRumble(SDL_GameControllerGetJoystick(controllerList[i]), left, right, 1000);
		}
	}
}

void __stdcall initManager() {
	printf("Initializing Manager!\n");

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

	loadInputSettings(&inputsettings);
	loadKeyBinds(&keybinds);
	loadControllerBinds(&padbinds);

	initSDLControllers();

	if (inputsettings.isPs2Controls) {
		registerPS2ControlPatch();
		patchPs2Buttons();
	}
}

// wrapper over the rolling friction calculation, provides easy access to the skater physics component
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
		push 0x005ceb7d	/* false */
		ret 0x08

	success:
		pop ecx
		pop eax
		mov esp, ebp
		pop ebp
		push 0x005ce9c8	/* true */
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

	patchByte((void *)(0x0046ee86 + 2), 0x05);	// change PC platform to ps2.  this just makes it default to ps2 controls
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
	patchByte((void *)(0x005d7d99 + 2), 0xa0);	// flip
	patchByte((void *)(0x005d7e9f + 2), 0xa0);	// spin

	// disable spin delay on l1/r1
	patchNop((void *)(0x005cd230), 2);
	patchNop((void *)(0x005cd2fb), 2);
}

void patchInput() {
	// patch SIO::Device
	// process
	patchThisToCdecl((void *)0x0062b090, &processController);	// 0062b240
	patchByte((void *)(0x0062b090 + 7), 0xC3);

	// set_actuator
	// don't call read_data in activate_actuators
	patchNop(0x0062ade1, 5);
	patchCall(0x0062ae64, set_actuators);
	patchCall(0x0062aec7, set_actuators);
	patchCall(0x0062af6f, set_actuators);
	patchCall(0x0062b001, set_actuators);
	patchCall(0x0062b05f, set_actuators);
	
	// init input patch - nop direct input setup
	patchNop(0x006b4c23, 45);
	patchCall(0x006b4c23 + 5, &initManager);

	// some config call relating to the dinput devices
	patchNop(0x0054481c, 5);
}