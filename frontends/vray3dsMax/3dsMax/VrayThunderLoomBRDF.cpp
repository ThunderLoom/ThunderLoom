//More Vray specific things. Implements methods for the BSDFSampler interface.

#include "Eval.h"
#include "helper.h"

#include "VrayThunderLoomBRDF.h"

using namespace VUtils;

//NOTE(Vidar): This function is called for each intersection with the material
// From this point, several rays can be fired in different directions, each
// one calling eval(). In this function we can do all the work that is common
// throughout all directions, such as computing the diffuse color.
void MyBaseBSDF::init(const VRayContext &rc, tlWeaveParameters *weave_parameters) {
    m_weave_parameters = weave_parameters;
    EvalDiffuseFunc(rc,weave_parameters,&diffuse_color,&m_yarn_type,
		&m_yarn_type_id);
    orig_backside = rc.rayresult.realBack;

    const VR::VRayInterface &vri_const=static_cast<const VR::VRayInterface&>(rc);
	VR::VRayInterface &vri=const_cast<VR::VRayInterface&>(vri_const);
	ShadeContext &sc=static_cast<ShadeContext&>(vri);

	// Set the normals to use for lighting
	normal=rc.rayresult.normal;
	gnormal=rc.rayresult.gnormal;
	if (rc.rayresult.realBack) {
		normal=-normal;
		gnormal=-gnormal;
	}
}

// From BRDFSampler

//NOTE(Vidar): Return the normal used for diffuse shading. If we do bump mapping
// on the diffuse color, we will have to return the modified normal here...
VUtils::ShadeVec MyBaseBSDF::getDiffuseNormal(const VR::VRayContext &rc)
{
    return rc.rayresult.origNormal;
}
VUtils::ShadeCol MyBaseBSDF::getDiffuseColor(VUtils::ShadeCol &lightColor) {
    VUtils::ShadeCol ret = diffuse_color*lightColor;
	lightColor.makeZero();
    return ret;
}
VUtils::ShadeCol MyBaseBSDF::getLightMult(VUtils::ShadeCol &lightColor) {
	/*
	tlColor s = m_yarn_type.specular_color;
	if(!m_yarn_type.specular_color_enabled
		&& m_tl_wparams->num_yarn_types > 0){
		s = m_tl_wparams->yarn_types[0].specular_color;
	}
	ShadeCol ret = (m_diffuse_color + ShadeCol(s.r,s.g,s.b)) * lightColor;
	*/
	//TODO(Vidar):What does this function do??
	ShadeCol ret = ShadeCol(1.0,1.0,1.0)* lightColor;
    lightColor.makeZero();
    return ret;
}
VUtils::ShadeCol MyBaseBSDF::getTransparency(const VRayContext &rc) {
	return VUtils::ShadeCol(0.f);
}

ShadeCol MyBaseBSDF::eval( const VRayContext &rc, const ShadeVec &direction, ShadeCol &shadowedLight,
	ShadeCol &origLight, float probLight,int flags)
{

    ShadeCol ret(0.f);

    float cs = direction * rc.rayresult.normal;
    cs = cs < 0.0f ? 0.0f : cs;

    if(flags & FBRDF_DIFFUSE){
        float k = cs;
        // Apply combined sampling ONLY if GI is on which will pick up the rest
        // of the result
        if (rc.rayparams.localRayType & RT_IS_GATHERING_POINT){
            float probReflection=k;
            //NOTE(Vidar): Multiple importance sampling factor
            k = cs*getReflectionWeight(probLight, probReflection);
        }
        ret += diffuse_color * k;
    }
    if((flags & FBRDF_SPECULAR) && ((rc.rayparams.rayType & RT_NOSPECULAR) == 0)){
        VUtils::ShadeCol reflect_color;
        //TODO(Vidar):Better importance sampling... Cosine weighted for now
        float probReflection=cs;
		EvalSpecularFunc(rc,direction,m_weave_parameters,&reflect_color);

        //NOTE(Vidar): Multiple importance sampling factor
        float weight = getReflectionWeight(probLight,probReflection);
        ret += cs*reflect_color*weight;
    }

    ret *= shadowedLight;

    shadowedLight.makeZero();
    origLight.makeZero();

    return ret;
}

//NOTE(Vidar): Called whenever a reflection ray is to be calculated
void MyBaseBSDF::traceForward(VRayContext &rc, int doDiffuse) {
	BRDFSampler::traceForward(rc, doDiffuse);
	if (doDiffuse) rc.mtlresult.color+=rc.evalDiffuse()*diffuse_color;
    Fragment *f=rc.mtlresult.fragment;
    if(f){
        VUtils::ShadeCol raw_gi=rc.evalDiffuse();
        VUtils::ShadeCol diffuse_gi=raw_gi*diffuse_color;
        f->setChannelDataByAlias(REG_CHAN_VFB_RAWGI,&raw_gi);
        f->setChannelDataByAlias(REG_CHAN_VFB_RAWTOTALLIGHT,&raw_gi);
        f->setChannelDataByAlias(REG_CHAN_VFB_GI,&diffuse_gi);
        f->setChannelDataByAlias(REG_CHAN_VFB_TOTALLIGHT,&diffuse_gi);
        f->setChannelDataByAlias(REG_CHAN_VFB_DIFFUSE,&diffuse_color);
    }
}

int MyBaseBSDF::getNumSamples(const VRayContext &rc, int doDiffuse) {
    if(rc.rayparams.currentPass==RPASS_GI ||
        (rc.rayparams.rayType & RT_LIGHT)!=0) {
        return 0;
    }
    //TODO(Vidar):Select using DMC sampler?
    return 8;
}

VRayContext* MyBaseBSDF::getNewContext(const VRayContext &rc, int &samplerID, int doDiffuse) {
    //doDiffuse == 0 => only specular
    //doDiffuse == 1 => specular & diffuse
    //doDiffuse == 2 => only diffuse

	//TODO(Vidar): Importance sampling!
	return NULL;
}

ValidType MyBaseBSDF::setupContext(const VRayContext &rc, VRayContext &nrc, float uc, int doDiffuse) {
    //TODO(Vidar):Better importance sampling... Cosine weighted for now
    ShadeVec dir = getDiffuseDir3f(uc, BRDFSampler::getDMCParam(nrc,1),nrc.rayparams.rayProbability);
    if (dir*gnormal<0.0f) return false;
    VR::getReflectDerivs(rc.rayparams.viewDir, dir, rc.rayparams.dDdx, rc.rayparams.dDdy, nrc.rayparams.dDdx, nrc.rayparams.dDdy);
    nrc.rayparams.tracedRay.dir=nrc.rayparams.viewDir=dir;
    return true;
}

RenderChannelsInfo* MyBaseBSDF::getRenderChannels(void) { return &RenderChannelsInfo::reflectChannels; }

//NOTE(Vidar):Return the correct BRDF depending on which side of the surface
// was hit...
BRDFSampler* MyBaseBSDF::getBRDF(BSDFSide side) {
    if (side==bsdfSide_front) {
        if (orig_backside) return NULL;
    } else {
        if (!orig_backside) return NULL;
    }
    return static_cast<BRDFSampler*>(this);
}
