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
#include "imtl.h"
#include "object.h"
#include "render.h"
#include "shadedata_new.h"

void EvalDiffuseFunc (const VUtils::VRayContext &rc,
    wcWeaveParameters *weave_parameters, VUtils::Color *diffuse_color,
	wcYarnType *yarn_type, int *yarn_type_id, wcPatternData *pattern_data);

void EvalSpecularFunc ( const VUtils::VRayContext &rc,
    const VUtils::Vector &direction, wcWeaveParameters *weave_parameters,
    VUtils::Matrix nm, VUtils::Color *reflection_color, int yarn_hit);