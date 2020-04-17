#include <Windows.h>

static HMODULE g_lib = 0;

HMODULE load_dynamic_library(void)
{
	if (g_lib) {
		return g_lib;
	}
	static char path[MAX_PATH] = { 0 };
	static int path_loaded = 0;
	if (!path_loaded) {
		HMODULE hm = NULL;
		if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
			GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(LPCSTR)&load_dynamic_library, &hm) == 0)
		{
			return 0;
		}
		if (GetModuleFileNameA(hm, path, sizeof(path)) == 0)
		{
			return 0;
		}
		char* last_slash = path;
		char* c = path;
		while (*c) {
			last_slash = *c == '/' || *c == '\\' ? c : last_slash;
			c++;
		}
		last_slash[1] = 0;
		path_loaded = 1;
	}

	FARPROC func = 0;

	SetDllDirectoryA(path);
	HMODULE lib = LoadLibraryA("thunderLoomVRay3dsMax" TL_MAX_VERSION "_dynamic.dll"); 
	SetDllDirectoryA(NULL);
	return lib;
}

void unload_dynamic_library(HMODULE lib)
{
	if (lib != g_lib) {
		FreeLibrary(lib);
	}
}

void lock_dynamic_library(void)
{
#ifdef MAX_DYNAMIC_LOAD
	g_lib = load_dynamic_library();
#endif
}

void unlock_dynamic_library(void)
{
#ifdef MAX_DYNAMIC_LOAD
	FreeLibrary(g_lib);
	g_lib = 0;
#endif
}

