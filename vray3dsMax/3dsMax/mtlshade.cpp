#include "3dsMaxThunderLoom.h"

#include "vraybase.h"
#include "vrayinterface.h"
#include "shadedata_new.h"
#include "tomax.h"
#include "vraycore.h"
#include "vrayrenderer.h"

//TODO(Peter): Clean up, gather up vray specific things to one file
		
VR::BSDFSampler* ThunderLoomMtl::newBSDF(const VR::VRayContext &rc, VR::VRenderMtlFlags flags) {
	MyBlinnBSDF *bsdf=bsdfPool.newBRDF(rc);
	if (!bsdf) return NULL;
    //NOTE(Vidar): Send pattern to BSDF
    bsdf->init(rc, toColor(this->reflect), this->glossiness, 8, VUtils::Color(0.f,0.f,0.f),//toColor(opacity).whiteComplement(),
        true, toColor(diffuse), &m_weave_parameters);
	return bsdf;
}

void ThunderLoomMtl::deleteBSDF(const VR::VRayContext &rc, VR::BSDFSampler *b) {
	if (!b) return;
	MyBlinnBSDF *bsdf=static_cast<MyBlinnBSDF*>(b);
	bsdfPool.deleteBRDF(rc, bsdf);
}

VR::VRayVolume* ThunderLoomMtl::getVolume(const VR::VRayContext &rc) {
	return NULL;
}

void ThunderLoomMtl::addRenderChannel(int index) {
	renderChannels+=index;
}

// The actual BRDF stuff

VR::Vector MyBlinnBSDF::getGlossyReflectionDir(float uc, float vc, const VR::Vector &viewDir, float &rayProbability) {

    return VR::getDiffuseDir(uc,vc,rayProbability);
	//return VR::blinnDir(uc, vc, glossiness, viewDir, nm, rayProbability);
}

VR::real MyBlinnBSDF::getGlossyProbability(const VR::Vector &direction, const VR::Vector &viewDir) {
    VR::Vector d(direction);
    return VR::getDiffuseDirProbInverse(d,normal);
	//return VR::blinnDirProb(direction, glossiness, viewDir, inm);
}

float MyBlinnBSDF::remapGlossiness(float nk) {
	return nk>0.9999f? -1.0f : (1.0f/powf(1.0f-nk, 3.5f)-1.0f);
}