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
	return Color(0.8f, 0.8f, 0.8f);
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
    bsdf->init(rc, &m_weave_parameters);
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
