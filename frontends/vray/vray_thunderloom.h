
#ifndef VRAY_THUNDERLOOM_H
#define VRAY_THUNDERLOOM_H

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
    VR::ShadeVec normal, gnormal;
    VR::ShadeMatrix nm, inm;
    int nmInited;

    // Assigned in init()
    tlWeaveParameters* m_tl_wparams;
    VR::ShadeVec m_uv;
    VR::ShadeTransform m_uv_tm;
    VR::ShadeCol m_diffuse_color;
    tlYarnType m_yarn_type;
    int m_yarn_type_id;

public:
    // Initialization
    void init(const VR::VRayContext &rc, tlWeaveParameters *weave_parameters);

    // From BRDFSampler
    VR::ShadeVec getDiffuseNormal(const VR::VRayContext &rc);
    VR::ShadeCol getDiffuseColor(VR::ShadeCol &lightColor);
    VR::ShadeCol getLightMult(VR::ShadeCol &lightColor);
    VR::ShadeCol getTransparency(const VR::VRayContext &rc);

    VR::ShadeCol eval( const VR::VRayContext &rc, const VR::ShadeVec &direction, VR::ShadeCol &shadowedLight, VR::ShadeCol &origLight, float probLight,int flags);
    void traceForward(VR::VRayContext &rc, int doDiffuse);

    int getNumSamples(const VR::VRayContext &rc, int doDiffuse);
    VR::VRayContext* getNewContext(const VR::VRayContext &rc, int &samplerID, int doDiffuse);
    VR::ValidType setupContext(const VR::VRayContext &rc, VR::VRayContext &nrc, float uc, int doDiffuse);

    VR::RenderChannelsInfo* getRenderChannels(void);

    // From BSDFSampler
    BRDFSampler *getBRDF(VR::BSDFSide side);
};


//Param helpers
FORCEINLINE int is_param_valid(VR::VRayPluginParameter* param, int i) {
    // Checks if there is a entry for the paremter with the given index i
    if (!param)
        return 0;

    int count = param->getCount(0);
    if (count == -1)
        count = 1;

    if (i >= count)
        return 0;

    return 1;
}

FORCEINLINE int set_texparam(VR::VRayPluginParameter* param, void **target, int i) {
    vassert(param != nullptr);
    // returns true if it is a texture and sets target to the texture pointer.
    VR::VRayParameterType type = param->getType(i, 0);

    if (type == VR::paramtype_object) {
        //TODO check if it has interface.
		PluginBase* newPlug = param->getObject(i);
		*target = newPlug;
        return 1;
    }
    return 0;
}


float tl_eval_texmap_mono(void* texmap, void* context);
float tl_eval_texmap_mono_lookup(void *texmap, float u, float v, void *context);
tlColor tl_eval_texmap_color(void *texmap, void *context);


/// Updated Version Below ///


struct BRDFThunderLoomParamsUpdated : VR::VRayParameterListDesc {
	BRDFThunderLoomParamsUpdated(void);
};

struct YarnCache {
    VR::VRayPluginList bend;
    VR::VRayPluginList yarnsize;
    VR::VRayPluginList twist;
    VR::VRayPluginList phase_alpha;
    VR::VRayPluginList phase_beta;
    VR::VRayPluginList specular_color_amount;
    VR::VRayPluginList specular_noise;
    VR::VRayPluginList highlight_width;
    VR::VRayPluginList diffuse_color_amount;

    VR::IntList bend_on;
    VR::IntList yarnsize_on;
    VR::IntList twist_on;
    VR::IntList phase_alpha_on;
    VR::IntList phase_beta_on;
    VR::IntList specular_color_on;
    VR::IntList specular_color_amount_on;
    VR::IntList specular_noise_on;
    VR::IntList highlight_width_on;
    VR::IntList diffuse_color_on;
    VR::IntList diffuse_color_amount_on;
};


struct BRDFThunderLoomUpdated: VR::VRayBSDF {
	BRDFThunderLoomUpdated(VR::VRayPluginDesc *pluginDesc);

    YarnCache yarnCache;
    tlWeaveParameters *m_tl_wparams;

    // From VRayBSDF
    void frameBegin(VR::VRayRenderer *vray);
    void frameEnd(VR::VRayRenderer *vray);

    // From BRDFInterface
    VR::BSDFSampler *newBSDF(const VR::VRayContext &rc, VR::BSDFFlags flags);
    void deleteBSDF(const VR::VRayContext &rc, VR::BSDFSampler *bsdf);

    virtual PluginInterface* newInterface(InterfaceID id) {
        if (id==EXT_BSDF) return static_cast<PluginInterface*>((VR::BSDFInterface*) this);
        return VR::VRayPlugin::newInterface(id);
    }

    int getBSDFFlags(void) {
        return VR::bsdfFlag_none;
    }

private:
    VR::BRDFPool<BRDFThunderLoomSampler> pool;
    VR::CharString m_filepath;
    float m_uscale, m_vscale, m_uvrotation;
    // Parameters
    PluginBase *baseBRDFParam;
};


#endif //VRAY_THUNDERLOOM_H