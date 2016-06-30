#ifndef __BLINN_BRDF_SAMPLER__
#define __BLINN_BRDF_SAMPLER__

#include "woven_cloth.h"

namespace VUtils {

class MyBaseBSDF: public BRDFSampler, public BSDFSampler {
protected:
	Color diffuse_color;

	Vector normal, gnormal;
    int orig_backside;

	Matrix nm, inm; // A matrix with the normal as the z-axis; can be used for anisotropy

    wcWeaveParameters *m_weave_parameters;

public:

	// Initialization
	void init(const VRayContext &rc, wcWeaveParameters *weave_parameters);

	// From BRDFSampler
	Vector getDiffuseNormal(const VR::VRayContext &rc);
	Color getDiffuseColor(Color &lightColor);
	Color getLightMult(Color &lightColor);
	Color getTransparency(const VRayContext &rc);

	Color eval(const VRayContext &rc, const Vector &direction, Color &lightColor, Color &origLightColor, float probLight, int flags);
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
