#ifndef __BLINN_BRDF_SAMPLER__
#define __BLINN_BRDF_SAMPLER__

#include "thunderloom.h"
#include "imtl.h"

namespace VUtils {

class MyBaseBSDF: public BRDFSampler, public BSDFSampler {
protected:
	ShadeCol m_diffuse_color;
	float m_diffuse_color_alpha;

	ShadeVec normal, gnormal;
    int orig_backside;

    tlWeaveParameters *m_weave_parameters;
	Texmap **m_texmaps;
	tlYarnType m_yarn_type;
	int m_yarn_type_id;

public:

	// Initialization
	void init(const VRayContext &rc, tlWeaveParameters *weave_parameters);

	// From BRDFSampler
	ShadeVec getDiffuseNormal(const VR::VRayContext &rc);
	ShadeCol getDiffuseColor(ShadeCol &lightColor);
	ShadeCol getLightMult(ShadeCol &lightColor);
	ShadeCol getTransparency(const VRayContext &rc);

	ShadeCol eval( const VRayContext &rc, const ShadeVec &direction, ShadeCol &shadowedLight, ShadeCol &origLight, float probLight,int flags);
	void traceForward(VRayContext &rc, int doDiffuse);

	int getNumSamples(const VRayContext &rc, int doDiffuse);
	VRayContext* getNewContext(const VRayContext &rc, int &samplerID, int doDiffuse);
	ValidType setupContext(const VRayContext &rc, VRayContext &nrc, float uc, int doDiffuse);

	RenderChannelsInfo* getRenderChannels(void);

	// From BSDFSampler
	BRDFSampler *getBRDF(BSDFSide side);

};

} // namespace VUtils

#endif
