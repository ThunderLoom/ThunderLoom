#pragma once

// Disable MSVC warnings for external headers.
#pragma warning( push )
#pragma warning( disable : 4251)
#pragma warning( disable : 4996 )
#include "vrayplugins.h"
#include "utils.h"
#include "vrayinterface.h"
#include "vrayrenderer.h"
#include "brdfs.h"
#include "vraytexutils.h"
#include "defparams.h"
#pragma warning( pop ) 

#include "thunderloom.h"

class BRDFThunderLoomSampler: public VR::BRDFSampler, public VR::BSDFSampler {
    int orig_backside;
    VUtils::ShadeVec normal, gnormal;
    VUtils::ShadeMatrix nm, inm;
    int nmInited;

    // Assigned in init()
    tlWeaveParameters* m_tl_wparams;
    VUtils::ShadeVec m_uv;
    VUtils::ShadeTransform m_uv_tm;
    VUtils::ShadeCol m_diffuse_color;
    tlYarnType m_yarn_type;
    int m_yarn_type_id;

public:
	// Initialization
	void init(const VUtils::VRayContext &rc, tlWeaveParameters *weave_parameters);

	// From BRDFSampler
	VUtils::ShadeVec getDiffuseNormal(const VR::VRayContext &rc);
	VUtils::ShadeCol getDiffuseColor(VUtils::ShadeCol &lightColor);
	VUtils::ShadeCol getLightMult(VUtils::ShadeCol &lightColor);
	VUtils::ShadeCol getTransparency(const VUtils::VRayContext &rc);

	VUtils::ShadeCol eval( const VUtils::VRayContext &rc, const VUtils::ShadeVec &direction, VUtils::ShadeCol &shadowedLight, VUtils::ShadeCol &origLight, float probLight,int flags);
	void traceForward(VUtils::VRayContext &rc, int doDiffuse);

	int getNumSamples(const VUtils::VRayContext &rc, int doDiffuse);
	VUtils::VRayContext* getNewContext(const VUtils::VRayContext &rc, int &samplerID, int doDiffuse);
	VUtils::ValidType setupContext(const VUtils::VRayContext &rc, VUtils::VRayContext &nrc, float uc, int doDiffuse);

	VUtils::RenderChannelsInfo* getRenderChannels(void);

	// From BSDFSampler
	BRDFSampler *getBRDF(VUtils::BSDFSide side);
};
