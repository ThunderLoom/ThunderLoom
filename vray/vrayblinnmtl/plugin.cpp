#include "vrayblinnmtl.h"

#define MYDLLEXPORT __declspec(dllexport)

HINSTANCE hInstance;
int controlsInit=FALSE;

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved) {
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

MYDLLEXPORT const TCHAR* LibDescription(void) { return STR_LIBDESC; }
MYDLLEXPORT int LibNumberClasses(void) { return 1; }

MYDLLEXPORT ClassDesc* LibClassDesc(int i) {
	switch(i) {
		case 0: return GetSkeletonMtlDesc();
		default: return 0;
	}
}

MYDLLEXPORT ULONG LibVersion() { return VERSION_3DSMAX; }
