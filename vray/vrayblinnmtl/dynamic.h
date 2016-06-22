#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include "vraybase.h"
#include "vrayinterface.h"
#include "vraycore.h"
#include "vrayrenderer.h"
#include "dbgprint.h"
#include "woven_cloth.h"

typedef void (*EVALDIFFUSEFUNC)(const VUtils::VRayContext &rc,
    wcWeaveParameters *weave_parameters, VUtils::Color *diffuse_color);

typedef void (*EVALSPECULARFUNC)( const VUtils::VRayContext &rc, const VUtils::Vector &direction,
    wcWeaveParameters *weave_parameters, VUtils::Matrix nm,
    VUtils::Color *reflection_color);

/*typedef void (*EVALFUNC)(const VUtils::VRayContext &rc, const Vector &direction,
    wcWeaveParameters *weave_parameters, Matrix nm, VUtils::Color *diffuse_color,
    VUtils::Color *reflection_color)*/


static void *get_dynamic_func(const char *fname)
{
#define DYNAMIC_DLL_NAME L"vrayblinnmtl_dynamic.dll" //NOTE(Vidar): Edit me to match the name of the dll!
#ifdef MAX_RELEASE_R18 
#define PLUGIN_PATH L"C:\\Program Files\\Autodesk\\3ds Max 2016\\plugins\\vrayplugins\\" //NOTE(Vidar) Coould use IPathConfigMgr::GetPlugInDesc() instead...
#else
#define PLUGIN_PATH L"C:\\Program Files\\Autodesk\\3ds Max 2015\\plugins\\vrayplugins\\"
#endif 
    DWORD len = GetDllDirectory(0,NULL);
    WCHAR *oldDir = new WCHAR[len];
    GetDllDirectory(len,oldDir);
    SetDllDirectory(PLUGIN_PATH);

    HINSTANCE oldHandle = GetModuleHandle(PLUGIN_PATH DYNAMIC_DLL_NAME);
    if(oldHandle){
        FreeLibrary(oldHandle);
    }
    //NOTE(Vidar) Add a breakpoint here if the function crashes and you want to unload the dlls
    //DebugPrint(L"Unloaded DLL's\n");

    HINSTANCE dynamic_dll = LoadLibrary(DYNAMIC_DLL_NAME);
    SetDllDirectory(oldDir);
    delete[] oldDir;
    void *func = NULL;
    if(dynamic_dll != NULL){
        func = GetProcAddress(dynamic_dll, fname);
        if(func == NULL){
            DWORD lastError = GetLastError();
            DebugPrint(L"Error loading function %s\nError code: %d\n",L"dynamic_test",lastError);
        }
    } else {
        DWORD lastError = GetLastError();
        DebugPrint(L"Loading " DYNAMIC_DLL_NAME L" failed!\nError code: %d\n",lastError);
    }
    return func;
}

static void unload_dlls()
{
    DWORD len = GetDllDirectory(0,NULL);
    WCHAR *oldDir = new WCHAR[len];
    GetDllDirectory(len,oldDir);
    SetDllDirectory(PLUGIN_PATH);

    HINSTANCE oldHandle = GetModuleHandle(PLUGIN_PATH DYNAMIC_DLL_NAME);
    if(oldHandle){
        FreeLibrary(oldHandle);
    }

    SetDllDirectory(oldDir);
    delete[] oldDir;
}

#ifndef DYNAMIC
void
EvalDiffuseFunc
(const VUtils::VRayContext &rc,
    wcWeaveParameters *weave_parameters, VUtils::Color *diffuse_color);
#endif

#ifndef DYNAMIC
void
EvalSpecularFunc
( const VUtils::VRayContext &rc, const VUtils::Vector &direction,
    wcWeaveParameters *weave_parameters, VUtils::Matrix nm,
    VUtils::Color *reflection_color);
#endif
