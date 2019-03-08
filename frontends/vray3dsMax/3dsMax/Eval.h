#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// Disable some MSVC warnings for external headers.
#pragma warning( push )
#pragma warning( disable : 4251)
#pragma warning( disable : 4996 )
#include "vraybase.h"
#include "vrayinterface.h"
#include "vraycore.h"
#include "vrayrenderer.h"
#include "dbgprint.h"
extern "C" {
#include "thunderloom.h"
}
#include "imtl.h"
#include "object.h"
#include "render.h"
#include "shadedata_new.h"
#pragma warning( pop ) 

void EvalDiffuseFunc (const VUtils::VRayContext &rc,
    tlWeaveParameters *weave_parameters, VUtils::ShadeCol *diffuse_color,
	tlYarnType *yarn_type, int *yarn_type_id);

void EvalSpecularFunc(const VUtils::VRayContext &rc,
	const VUtils::ShadeVec &direction, tlWeaveParameters *weave_parameters, VUtils::ShadeCol *reflection_color) ;
