#ifndef _SCRIPT_H_
#define _SCRIPT_H_

uint8_t get_script_offsets();
void patchScriptHook();
void initScriptPatches();
void registerPS2ControlPatch();
void setMenuControls(uint8_t value);

uint8_t wrap_cfunc(char* name, void* wrapper, void** addr_out);
#endif