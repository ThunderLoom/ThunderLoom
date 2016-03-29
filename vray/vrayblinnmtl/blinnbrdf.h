#ifndef __BLINN_BRDF_SAMPLER__
#define __BLINN_BRDF_SAMPLER__

#include "woven_cloth.h"

namespace VUtils {

class MyBaseBSDF: public BRDFSampler, public BSDFSampler {
protected:
	Color reflect_filter, transparency;
	real glossiness;
	int nsamples;
	int doubleSided;
	Color diffuse_color;

	int dontTrace; // true if we need to trace reflections and false otherwise
	int origBackside;

	Vector normal, gnormal;

	Matrix nm, inm; // A matrix with the normal as the z-axis; can be used for anisotropy

    wcWeaveParameters *m_weave_parameters;

	virtual void computeNormalMatrix(const VRayContext &rc, const Vector &normal, Matrix &nm);
public:
	// Methods required for derived classes
	virtual Vector getGlossyReflectionDir(float uc, float vc, const Vector &viewDir, float &rayProbability)=0;
	virtual VUtils::real getGlossyProbability(const Vector &direction, const Vector &viewDir)=0;
	virtual float remapGlossiness(float nk)=0;

	// Initialization
	void init(const VRayContext &rc, const Color &reflectionColor, real reflectionGlossiness,
        int subdivs, const Color &transp, int dblSided, const Color &diffuse,
        wcWeaveParameters *weave_parameters);

	// From BRDFSampler
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
