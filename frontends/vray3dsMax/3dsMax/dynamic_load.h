#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

extern "C" HMODULE load_dynamic_library(void)
;
extern "C" void unload_dynamic_library(HMODULE)
;
extern "C" void lock_dynamic_library(void)
;
extern "C" void unlock_dynamic_library(void)
;

#ifdef MAX_DYNAMIC_LOAD
#define CALL_DYNAMIC_FUNC(name, ret_type, ret_name) CALL_DYNAMIC_FUNC_(name, ret_type, ret_name, 0)
#define CALL_DYNAMIC_FUNC_VOID(name) CALL_DYNAMIC_FUNC_(name, int, unused, 1)
#define CALL_DYNAMIC_FUNC_(name, ret_type, ret_name, is_void) \
ret_type ret_name;\
{\
	HMODULE lib = load_dynamic_library(); \
	if (lib) { \
		FARPROC func = GetProcAddress(lib, #name); \
		if (func) { \
			if(is_void){\
				((void(*)(DYNAMIC_FUNC_ARG_TYPES)) func)(DYNAMIC_FUNC_ARG_NAMES); \
			}\
			else {\
				ret_name = ((ret_type(*)(DYNAMIC_FUNC_ARG_TYPES)) func)(DYNAMIC_FUNC_ARG_NAMES); \
			}\
		} \
		else { \
			OutputDebugStringA("Could not load function from DLL!");\
			/*the_listener->edit_stream->printf(L"Error loading DLL\n");*/ \
		} \
		unload_dynamic_library(lib); \
	} \
	else { \
		OutputDebugStringA("Could not load DLL!");\
		/*the_listener->edit_stream->printf(L"Could not load DLL\n");*/ \
	} \
}
#define DYNAMIC_FUNC_PREFIX extern "C" __declspec(dllexport)
#else
#define DYNAMIC_FUNC_PREFIX
#define CALL_DYNAMIC_FUNC(name, ret_type, ret_name) ret_type ret_name = name(DYNAMIC_FUNC_ARG_NAMES);
#define CALL_DYNAMIC_FUNC_VOID(name) name(DYNAMIC_FUNC_ARG_NAMES);
#endif
