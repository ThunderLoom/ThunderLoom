#include "3dsMaxThunderLoom.h"

#define MYDLLEXPORT __declspec(dllexport)

HINSTANCE hInstance;
int controlsInit=FALSE;

// This function is called by Windows when the DLL is loaded.  This 
// function may also be called many times during time critical operations
// like rendering.  Therefore developers need to be careful what they
// do inside this function.  In the code below, note how after the DLL is
// loaded the first time only a few statements are executed.
BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved) {
	// Hang on to this DLL's instance handle.
	hInstance = hinstDLL;

	if ( !controlsInit ) {
		controlsInit = TRUE;
#if MAX_RELEASE<13900
		InitCustomControls(hInstance);
#endif
		InitCommonControls();
	}

	switch(fdwReason) {
		case DLL_PROCESS_ATTACH:
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return(TRUE);
}

// This function returns a string that describes the DLL and where the user
// could purchase the DLL if they don't have it.
MYDLLEXPORT const TCHAR* LibDescription(void) { return STR_LIBDESC; }

// This function returns the number of plug-in classes this DLL
//TODO: Must change this number when adding a new class
MYDLLEXPORT int LibNumberClasses(void) { return 1; }

// This function returns the number of plug-in classes this DLL
MYDLLEXPORT ClassDesc* LibClassDesc(int i) {
	switch(i) {
		case 0: return GetSkeletonMtlDesc();
		default: return 0;
	}
}

// This function returns a pre-defined constant indicating the version of 
// the system under which it was compiled.  It is used to allow the system
// to catch obsolete DLLs.
MYDLLEXPORT ULONG LibVersion() { return VERSION_3DSMAX; }
