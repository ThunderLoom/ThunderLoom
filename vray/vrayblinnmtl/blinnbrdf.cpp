#include "dynamic.h"
#include "vraybase.h"
#include "vrayinterface.h"
#include "vraycore.h"
#include "vrayrenderer.h"

#include "blinnbrdf.h"

extern EVALFUNC EvalFunc;

using namespace VUtils;

void MyBaseBSDF::init(const VRayContext &rc, const Color &reflectionColor, real reflectionGlossiness,
                      int subdivs, const Color &transp, int dblSided, const Color &diffuse,
                      wcWeaveParameters *weave_parameters) {
    

	reflect_filter=reflectionColor;
	diffuse_color=diffuse;
	transparency=transp;
	doubleSided=dblSided;
	origBackside=rc.rayresult.realBack;

    m_weave_parameters = weave_parameters;

	// Dim the diffuse color by the reflection
	if (nsamples>0) {
		diffuse_color*=reflect_filter.whiteComplement();
	}

	// Dim the diffuse/reflection colors by the transparency
	Color invTransp=transp.whiteComplement();
	diffuse_color*=invTransp;
	reflect_filter*=invTransp;

	const VR::VRaySequenceData &sdata=rc.vray->getSequenceData();

	// Set the normals to use for lighting
	normal=rc.rayresult.normal;
	gnormal=rc.rayresult.gnormal;
	if (rc.rayresult.realBack) {
		normal=-normal;
		gnormal=-gnormal;
	}

	// Check if we need to trace the reflection
	dontTrace=p_or(
		p_or(rc.rayparams.totalLevel>=sdata.params.options.mtl_maxDepth, 0==sdata.params.options.mtl_reflectionRefraction),
		p_and(0!=(rc.rayparams.rayType & RT_INDIRECT), !sdata.params.gi.reflectCaustics)
	);

	if (dontTrace) {
		reflect_filter.makeZero();
	} else {
		glossiness=remapGlossiness(reflectionGlossiness);
		nsamples=subdivs*subdivs;

		// Compute the normal matrix
		if (rc.rayparams.viewDir*normal>0.0f) computeNormalMatrix(rc, gnormal, nm);
		else computeNormalMatrix(rc, normal, nm);

		inm=inverse(nm);
	}
}

// From BRDFSampler
Color MyBaseBSDF::getDiffuseColor(Color &lightColor) {
	Color res=diffuse_color*lightColor;
	lightColor*=transparency;
	return res;
}

Color MyBaseBSDF::getLightMult(Color &lightColor) {
	Color res=(diffuse_color+reflect_filter)*lightColor;
	lightColor*=transparency;
	return res;
}

Color MyBaseBSDF::eval(const VRayContext &rc, const Vector &direction, Color &lightColor, Color &origLightColor, float probLight, int flags) {
#ifdef DYNAMIC
    if(EvalFunc){
        float cs=(float) (direction*normal);
		if (cs<0.0f) cs=0.0f;
        return cs*EvalFunc(rc,direction,lightColor,origLightColor,probLight,flags,m_weave_parameters,nm);
    } else {
        return Color(1.f,0.f,1.f);
    }
#else
    float cs=(float) (direction*normal);
	if (cs<0.0f) cs=0.0f;
    return cs*dynamic_eval(rc,direction,lightColor,origLightColor,probLight,flags,m_weave_parameters,nm);
#endif
    /*
	// Skip this part if diffuse component is not required
	if (0!=(flags & FBRDF_DIFFUSE)) {
		float cs=(float) (direction*normal);
		if (cs<0.0f) cs=0.0f;
		float probReflection=2.0f*cs;

		float k=getReflectionWeight(probLight, probReflection);
		res+=(0.5f*probReflection*k)*(diffuse_color*lightColor);
	}

	// Skip this part if specular component is not required
	if (!dontTrace && glossiness>=0.0f && 0!=(flags & FBRDF_SPECULAR)) {
		float probReflection=getGlossyProbability(direction, rc.rayparams.viewDir);
		float k=getReflectionWeight(probLight, probReflection);
		res+=(0.5f*probReflection*k)*(reflect_filter*lightColor);
	}

	lightColor*=transparency;
	origLightColor*=transparency;

	return res;*/
}

void MyBaseBSDF::traceForward(VRayContext &rc, int doDiffuse) {
	BRDFSampler::traceForward(rc, doDiffuse);
	if (doDiffuse) rc.mtlresult.color+=rc.evalDiffuse()*diffuse_color;
}

int MyBaseBSDF::getNumSamples(const VRayContext &rc, int doDiffuse) {
	return p_or(rc.rayparams.currentPass==RPASS_GI, (rc.rayparams.rayType & RT_LIGHT)!=0, glossiness<0.0f)? 0 : nsamples;
}

Color MyBaseBSDF::getTransparency(const VRayContext &rc) {
	return transparency;
}

VRayContext* MyBaseBSDF::getNewContext(const VRayContext &rc, int &samplerID, int doDiffuse) {
	if (2==doDiffuse || dontTrace || nsamples==0) return NULL;

	// Create a new context
	VRayContext &nrc=rc.newSpawnContext(2, reflect_filter, RT_REFLECT | RT_GLOSSY | RT_ENVIRONMENT, normal);

	// Set up the new context
	nrc.rayparams.dDdx.makeZero(); // Zero out the directional derivatives
	nrc.rayparams.dDdy.makeZero();
	nrc.rayparams.mint=0.0f; // Set the ray extents
	nrc.rayparams.maxt=1e18f;
	nrc.rayparams.tracedRay.p=rc.rayresult.wpoint; // Set the new ray origin to be the surface hit point
	return &nrc;
}

ValidType MyBaseBSDF::setupContext(const VRayContext &rc, VRayContext &nrc, float uc, int doDiffuse) {
	if (glossiness<0.0f) {
		// Pure reflection
		Vector dir=getReflectDir(rc.rayparams.viewDir, rc.rayresult.normal);

		// If the reflection direction is below the surface, use the geometric normal
		real r0=-(real) (rc.rayparams.viewDir*rc.rayresult.gnormal);
		real r1=(real) (dir*rc.rayresult.gnormal);
		if (r0*r1<0.0f) dir=getReflectDir(rc.rayparams.viewDir, rc.rayresult.gnormal);

		// Set ray derivatives
		VR::getReflectDerivs(rc.rayparams.viewDir, dir, rc.rayparams.dDdx, rc.rayparams.dDdy, nrc.rayparams.dDdx, nrc.rayparams.dDdy);

		// Set the direction into the ray context
		nrc.rayparams.tracedRay.dir=nrc.rayparams.viewDir=dir;
		nrc.rayparams.tracedRay.p=rc.rayresult.wpoint; // Set the new ray origin to be the surface hit point
		nrc.rayparams.mint=0.0f; // Set the ray extents
		nrc.rayparams.maxt=1e18f;
	} else {
		// Compute a Blinn reflection direction
		Vector dir=getGlossyReflectionDir(uc, BRDFSampler::getDMCParam(nrc, 1), rc.rayparams.viewDir, nrc.rayparams.rayProbability);


		// If this is below the surface, ignore
		if (dir*gnormal<0.0f) return false;

		// Set ray derivatives
		VR::getReflectDerivs(rc.rayparams.viewDir, dir, rc.rayparams.dDdx, rc.rayparams.dDdy, nrc.rayparams.dDdx, nrc.rayparams.dDdy);

		// Set the direction into the ray context
		nrc.rayparams.tracedRay.dir=nrc.rayparams.viewDir=dir;
	}

	return true;
}

RenderChannelsInfo* MyBaseBSDF::getRenderChannels(void) { return &RenderChannelsInfo::reflectChannels; }

void MyBaseBSDF::computeNormalMatrix(const VR::VRayContext &rc, const VR::Vector &normal, VR::Matrix &nm) {
		makeNormalMatrix(normal, nm);
}

// From BSDFSampler
BRDFSampler* MyBaseBSDF::getBRDF(BSDFSide side) {
	if (!doubleSided) {
		if (side==bsdfSide_back) return NULL; // There is nothing on the back side
		return static_cast<VR::BRDFSampler*>(this);
	} else {
		if (side==bsdfSide_front) {
			if (origBackside) return NULL;
		} else {
			if (!origBackside) return NULL;
		}
		return static_cast<BRDFSampler*>(this);
	}
}
