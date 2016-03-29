#include "vrayblinnmtl.h"

#include "vraybase.h"
#include "vrayinterface.h"
#include "shadedata_new.h"
#include "tomax.h"
#include "vraycore.h"
#include "vrayrenderer.h"


/*===========================================================================*\
 |	Determine the characteristics of the material
\*===========================================================================*/

void SkeletonMaterial::SetAmbient(Color c, TimeValue t) {}		
void SkeletonMaterial::SetDiffuse(Color c, TimeValue t) {}		
void SkeletonMaterial::SetSpecular(Color c, TimeValue t) {}
void SkeletonMaterial::SetShininess(float v, TimeValue t) {}
				
Color SkeletonMaterial::GetAmbient(int mtlNum, BOOL backFace) {
	return Color(0,0,0);
}

Color SkeletonMaterial::GetDiffuse(int mtlNum, BOOL backFace) {
	return Color(0.5f, 0.5f, 0.5f);
}

Color SkeletonMaterial::GetSpecular(int mtlNum, BOOL backFace) {
	return Color(0,0,0);
}

float SkeletonMaterial::GetXParency(int mtlNum, BOOL backFace) {
	return 0.0f;
}

float SkeletonMaterial::GetShininess(int mtlNum, BOOL backFace) {
	return 0.0f;
}

float SkeletonMaterial::GetShinStr(int mtlNum, BOOL backFace) {
	return 0.0f;
}

float SkeletonMaterial::WireSize(int mtlNum, BOOL backFace) {
	return 0.0f;
}

		
/*===========================================================================*\
 |	Actual shading takes place
\*===========================================================================*/

void SkeletonMaterial::Shade(ShadeContext &sc) {
	if (sc.ClassID()==VRAYCONTEXT_CLASS_ID)
		shade(static_cast<VR::VRayInterface&>(sc), gbufID);
	else {
		if (gbufID) sc.SetGBufferID(gbufID);
		sc.out.c.Black(); sc.out.t.Black();
	}
}

float SkeletonMaterial::EvalDisplacement(ShadeContext& sc)
{
	return 0.0f;
}

Interval SkeletonMaterial::DisplacementValidity(TimeValue t)
{
	Interval iv;
	iv.SetInfinite();
	return iv;
}
	
VR::BSDFSampler* SkeletonMaterial::newBSDF(const VR::VRayContext &rc, VR::VRenderMtlFlags flags) {
	MyBlinnBSDF *bsdf=bsdfPool.newBRDF(rc);
	if (!bsdf) return NULL;
    //NOTE(Vidar): Send pattern to BSDF
    bsdf->init(rc, toColor(this->reflect), this->glossiness, 8, VUtils::Color(0.f,0.f,0.f),//toColor(opacity).whiteComplement(),
        true, toColor(diffuse), &m_weave_parameters);
	return bsdf;
}

void SkeletonMaterial::deleteBSDF(const VR::VRayContext &rc, VR::BSDFSampler *b) {
	if (!b) return;
	MyBlinnBSDF *bsdf=static_cast<MyBlinnBSDF*>(b);
	bsdfPool.deleteBRDF(rc, bsdf);
}

VR::VRayVolume* SkeletonMaterial::getVolume(const VR::VRayContext &rc) {
	return NULL;
}

void SkeletonMaterial::addRenderChannel(int index) {
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
