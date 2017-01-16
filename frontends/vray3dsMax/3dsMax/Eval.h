#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include "vraybase.h"
#include "vrayinterface.h"
#include "vraycore.h"
#include "vrayrenderer.h"
#include "dbgprint.h"
#include "thunderloom.h"
#include "imtl.h"
#include "object.h"
#include "render.h"
#include "shadedata_new.h"

void EvalDiffuseFunc (const VUtils::VRayContext &rc,
    tlWeaveParameters *weave_parameters, VUtils::Color *diffuse_color,
	tlYarnType *yarn_type, int *yarn_type_id);

void EvalSpecularFunc ( const VUtils::VRayContext &rc,
    const VUtils::Vector &direction, tlWeaveParameters *weave_parameters,
    VUtils::Matrix nm, VUtils::Color *reflection_color);