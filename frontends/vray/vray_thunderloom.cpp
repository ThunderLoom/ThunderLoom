// test plugin

#include "vrayplugins.h"
#include "vrayinterface.h"
#include "vrayrenderer.h"
#include "brdfs.h"
#include "vraytexutils.h"
// include "basetexture.h"
#include "defparams.h"

//#define TL_NO_TEXTURE_CALLBACKS
#define TL_THUNDERLOOM_IMPLEMENTATION
#include "thunderloom.h"

namespace VUtils{
class BRDFThunderLoomSampler: public BRDFSampler, public BSDFSampler {
//protected:

    int orig_backside;
    Vector normal, gnormal;
    Matrix nm, inm;

    // Assigned in init()
    tlWeaveParameters* m_tl_wparams;
    Vector m_uv;
    Transform m_uv_tm;
    Color m_diffuse_color;
    tlYarnType m_yarn_type;
    int m_yarn_type_id;

public:
    void init(const VR::VRayContext &rc, tlWeaveParameters *tl_wparams);

    //void

    // From BRDFSampler
    Vector getDiffuseNormal(const VR::VRayContext &rc);
    Color getDiffuseColor(Color &lightColor);
    Color getLightMult(Color &lightColor);
    Color getTransparency(const VR::VRayContext &rc);

    Color eval(const VR::VRayContext &rc, const Vector &direction, Color &lightColor, Color &origLightColor, float probLight, int flags);
    void traceForward(VR::VRayContext &rc, int doDiffuse);

    int getNumSamples(const VR::VRayContext &rc, int doDiffuse);
    VR::VRayContext* getNewContext(const VR::VRayContext &rc, int &samplerID, int doDiffuse);
    VR::ValidType setupContext(const VR::VRayContext &rc, VRayContext &nrcm, float uc, int doDiffuse);

    BRDFSampler* getBRDF(BSDFSide side);
    
    RenderChannelsInfo* getRenderChannels(void);

};

}

using namespace VR;

struct BRDFThunderLoomParams: VRayParameterListDesc {
    BRDFThunderLoomParams(void) {
        //addParamPlugin("test_bsdf", EXT_BSDF, -1, "A Test BSDF.");
        //addParamFloat("testfloat", 1.0f, -1, "A Test parameter.");
        addParamString("filepath", "", -1, "File path to pattern file");
        addParamFloat("uscale", 1.f, -1, "Scalefactor for u coordinate");
        addParamFloat("vscale", 1.f, -1, "Scalefactor for v coordinate");
        
        // Yarn settings.
        // These are to be stored as lists in the .vrscene file. The index in
        // the list corresponds to the yarn_type.
        addParamTextureFloat("bend", 0.5, -1, "How much each visible yarn segment gets bent.");
        addParamTextureFloat("yarnsize", 1.0, -1, "Width of yarn.");
        addParamTextureFloat("twist", 0.5, -1, "How strongly to simulate the twisting nature of the yarn. Usually synthetic yarns, such as polyester, have less twist than organic yarns, such as cotton.");
        addParamTexture("specular_color", Color(0.4f, 0.4f, 0.4f), -1, "Color of the specular reflections. This will implicitly affect the strength of the diffuse reflections.");
        addParamTextureFloat("specular_noise", 0.4, -1, "Noise on specular reflections.");
        addParamTextureFloat("highlight_width", 0.4, -1, "Width over which to average the specular reflections. Gives wider highlight streaks on the yarns.");
        //addParamColor("diffuse_color", Color(0.f, 0.3f, 0.f), -1, "Diffuse color.");
        addParamTexture("diffuse_color", Color(0.f, 0.3f, 0.f), -1, "Diffuse color.");
        
        // Stored as lists, just like above. These parameters allow us to 
        // specify what parameters we want to override, for a specific yarn.
        addParamBool("bend_on",                 false, -1, "");
        addParamBool("yarnsize_on",             false, -1, "");
        addParamBool("twist_on",                false, -1, "");
        addParamBool("specular_color_on",       false, -1, "");
        addParamBool("specular_noise_on",       false, -1, "");
        addParamBool("highlight_width_on",      false, -1, "");
        addParamBool("diffuse_color_on",        false, -1, "");

        // MAYA FIX
        // It seems that only float params can be retrieved from Maya into the 
        // vrscene file. And only as a texture if the floats are put into an 
        // array. We allow the _on params to be specified as a float aswell.
        addParamTextureFloat("bend_on_float",                 false, -1, "");
        addParamTextureFloat("yarnsize_on_float",             false, -1, "");
        addParamTextureFloat("twist_on_float",                false, -1, "");
        addParamTextureFloat("specular_color_on_float",       false, -1, "");
        addParamTextureFloat("specular_noise_on_float",       false, -1, "");
        addParamTextureFloat("highlight_width_on_float",      false, -1, "");
        addParamTextureFloat("diffuse_color_on_float",        false, -1, "");
        
        addParamFloat("bends2", 0.5, -1, "");
    }
};

//Struct for intermediatly storing yarn parameters.
typedef struct {
    float umax;
    float yarnsize;
    float psi;
    float alpha;
    float beta;
    float delta_x;
    Color specular_color;
    float specular_noise;
    Color color;
} tlIntermediateYrnParam;

struct BRDFThunderLoom: VRayBSDF {
    BRDFThunderLoom(VRayPluginDesc *pluginDesc):VRayBSDF(pluginDesc), bends2() {
        //bends("bend_vals");
        paramList->setParamCache("filepath", &m_filepath);
        paramList->setParamCache("uscale", &m_uscale);
        paramList->setParamCache("vscale", &m_vscale);
        
        paramList->setParamCache("bend", &yrn0.umax);
        paramList->setParamCache("yarnsize", &yrn0.yarnsize);
        paramList->setParamCache("twist", &yrn0.psi);
        paramList->setParamCache("highlight_width", &yrn0.delta_x);
        paramList->setParamCache("specular_color", &yrn0.specular_color);
        paramList->setParamCache("specular_noise", &yrn0.specular_noise);
        paramList->setParamCache("diffuse_color", &yrn0.color);
        
        /*paramList->setParamCache("bend_on", &yrn0.umax);
        paramList->setParamCache("yarnsize_on", &yrn0.yarnsize);
        paramList->setParamCache("twist_on", &yrn0.psi);
        paramList->setParamCache("highlight_width_on", &yrn0.delta_x);
        paramList->setParamCache("specular_strength_on", &yrn0.specular_strength);
        paramList->setParamCache("specular_noise_on", &yrn0.specular_noise);
        paramList->setParamCache("diffuse_color_on", &yrn0.color);*/

    }

    tlWeaveParameters *m_tl_wparams;
    tlIntermediateYrnParam yrn0;

    // From VRayBSDF
    void renderBegin(VRayRenderer *vray) {return;};
    void renderEnd(VRayRenderer *vray) {return;};
    void frameBegin(VRayRenderer *vray);
    void frameEnd(VRayRenderer *vray);

    // From BRDFInterface
    BSDFSampler *newBSDF(const VRayContext &rc, BSDFFlags flags);
    void deleteBSDF(const VRayContext &rc, BSDFSampler *bsdf);

    virtual PluginInterface* newInterface(InterfaceID id) {
        if (id==EXT_BSDF) return static_cast<PluginInterface*>((BSDFInterface*) this);
        return VRayPlugin::newInterface(id);
    }

    int isOpaque(void) VRAY_OVERRIDE {
        return getBRDFPluginFlags(baseBRDFParam);
    }

private:
    BRDFPool<BRDFThunderLoomSampler> pool;
    float testfloat;
    CharString m_filepath;
    float m_uscale, m_vscale;
    //DefFloatListParam bends;
    FloatList bends2;

    // Parameters
    PluginBase *baseBRDFParam;
    PluginBase *bumpTexPlugin;
};

//Param helpers
inline int is_param_valid(VRayPluginParameter* param, int i) {
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

inline int set_texparam(VRayPluginParameter* param, void **target, int i) {
    // returns true if it is a texture and sets target to the texture pointer.
    VRayParameterType type = param->getType(i, 0);
    const char* name = param->getName();

    if (type == paramtype_object) {
        //TODO check if it has interface.
        
        PluginBase* newPlug=param->getObject(i);
        //const char* plugName = newPlug->getName();
        TextureInterface *newSub=static_cast<TextureInterface *>(GET_INTERFACE(newPlug, EXT_TEXTURE));
        TextureFloatInterface *newSubfloat=static_cast<TextureFloatInterface *>(GET_INTERFACE(newPlug, EXT_TEXTURE_FLOAT));
        
        *target = newPlug;
        /*if (newSub)
            *target = newSub;
        if (newSubfloat)
            *target = newSubfloat;*/
        return 1;
    }
    return 0;
}

int get_bool(VRayPluginParameter * param, int i, VRayContext& rc) {
    VRayParameterType type = param->getType(i, 0);
    const char* name = param->getName();
    
    if (param->getType(i,0) == VRayParameterType::paramtype_bool && param->getBool(i))
      return 1;
    
    // For maya support ([param]_on_float), float arrays are sent as textures.
    void* tex;
    if (set_texparam(param, &tex, i)) {
        // param was texture and text is a pointer to it.
        // Get texture value
        if (tl_eval_texmap_mono_lookup(tex, 0.f, 0.f, &rc))
            return 1;
    }

    return 0;
}

void BRDFThunderLoom::frameBegin(VRayRenderer *vray) {
    // Get filepath
    VRayPluginParameter* filepath_param = this->getParameter("filepath");
    CharString filepath = filepath_param->getString();

    //Check for file, at path, then in VRAY_ASSETS_PATH and so forth.
    VUtils::checkAssetPath(filepath, vray->getSequenceData());
	
    //Check again, if exits and abort if not.
    VUtils::ProgressCallback *prog = vray->getSequenceData().progress;
    if (!VUtils::checkAssetPath(filepath, prog, true))
        return;


    // Load file and set the global settings.
    const char* filepath_str = (char*)filepath.ptr();
    const char *errors = 0;
    m_tl_wparams = tl_weave_pattern_from_file(filepath_str, &errors);
    if (errors) {
        prog->error("Error: ThunderLoom: %s", errors);
        return; //Abort a littel ncier
    }
    if (m_tl_wparams) {
        m_tl_wparams->uscale = 1.f;
        m_tl_wparams->vscale = 1.f;
        m_tl_wparams->realworld_uv = 0;
        tl_prepare(m_tl_wparams);
    }
    
    // Make new VRayContext
    VRayThreadData* vr_tdata = vray->getThreadData(0);
    VRayCore* vcore = static_cast<VRayCore *>(vray);
    VRayContext rc;
    rc.init(vr_tdata, vcore);
    
    VRayPluginParameter* uscale = this->getParameter("uscale");
    VRayPluginParameter* vscale = this->getParameter("vscale");
    m_tl_wparams->uscale = uscale->getFloat();
    m_tl_wparams->vscale = vscale->getFloat();

    // Yarn type params
    VRayPluginParameter* bend = this->getParameter("bend");
    VRayPluginParameter* bend_on = this->getParameter("bend_on");
    VRayPluginParameter* bend_on_float = this->getParameter("bend_on_float");

    VRayPluginParameter* yarnsize = this->getParameter("yarnsize");
    VRayPluginParameter* yarnsize_on = this->getParameter("yarnsize_on");
    VRayPluginParameter* yarnsize_on_float = this->getParameter("yarnsize_on_float");

    VRayPluginParameter* twist = this->getParameter("twist");
    VRayPluginParameter* twist_on = this->getParameter("twist_on");
    VRayPluginParameter* twist_on_float = this->getParameter("twist_on_float");

    VRayPluginParameter* specular_color = this->getParameter("specular_color");
    VRayPluginParameter* specular_color_on = this->getParameter("specular_color_on");
    VRayPluginParameter* specular_color_on_float = this->getParameter("specular_color_on_float");

    VRayPluginParameter* specular_noise = this->getParameter("specular_noise");
    VRayPluginParameter* specular_noise_on = this->getParameter("specular_noise_on");
    VRayPluginParameter* specular_noise_on_float = this->getParameter("specular_noise_on_float");

    VRayPluginParameter* highlight_width = this->getParameter("highlight_width");
    VRayPluginParameter* highlight_width_on = this->getParameter("highlight_width_on");
    VRayPluginParameter* highlight_width_on_float = this->getParameter("highlight_width_on_float");
    
    VRayPluginParameter* diffuse_color = this->getParameter("diffuse_color");
    VRayPluginParameter* diffuse_color_on = this->getParameter("diffuse_color_on");
    VRayPluginParameter* diffuse_color_on_float = this->getParameter("diffuse_color_on_float");


    // Loop through yarn types and set parameters from list
    int n = m_tl_wparams->num_yarn_types;
    for (int i=0; i < m_tl_wparams->num_yarn_types; i++) {
        //get parameter from config string
        tlYarnType* yarn_type = &m_tl_wparams->yarn_types[i];

#define TL_VRAY_SET_PARAM(name,tl_name) \
        if (is_param_valid(name, i)) {\
            if (!set_texparam(name, &yarn_type->tl_name##_texmap, i))\
                yarn_type->tl_name = name->getFloat(i);\
        }\
        if (is_param_valid(name##_on, i)) \
            yarn_type->tl_name##_enabled = get_bool(name##_on, i, rc); \
        else if (is_param_valid(name##_on_float, i)) \
            yarn_type->tl_name##_enabled = get_bool(name##_on_float, i, rc);

#define TL_VRAY_FLOAT_PARAMS\
        TL_VRAY_SET_PARAM(bend, umax)\
        TL_VRAY_SET_PARAM(yarnsize, yarnsize)\
        TL_VRAY_SET_PARAM(twist, psi)\
        TL_VRAY_SET_PARAM(highlight_width, delta_x)\
        TL_VRAY_SET_PARAM(specular_noise, specular_noise)\

TL_VRAY_FLOAT_PARAMS

#undef TL_VRAY_FLOAT_PARAMS
#undef TL_VRAY_SET_PARAM

        if (is_param_valid(specular_color, i)) {
            if (!set_texparam(specular_color, &yarn_type->specular_color_texmap, i)) {
                Color tmp_specular_color = specular_color->getColor(i);
                tlColor tl_specular_color;
                tl_specular_color.r = tmp_specular_color.r; tl_specular_color.g = tmp_specular_color.g; tl_specular_color.b = tmp_specular_color.b;
                yarn_type->color = tl_specular_color;
            }
        }
        if (is_param_valid(specular_color_on, i))
            yarn_type->specular_color_enabled = get_bool(specular_color_on, i, rc);
        else if (is_param_valid(specular_color_on_float, i))
            yarn_type->specular_color_enabled = get_bool(specular_color_on_float, i, rc);

        if (is_param_valid(diffuse_color, i)) {
            if (!set_texparam(diffuse_color, &yarn_type->color_texmap, i)) {
                Color tmp_diffuse_color = diffuse_color->getColor(i);
                tlColor tl_diffuse_color;
                tl_diffuse_color.r = tmp_diffuse_color.r; tl_diffuse_color.g = tmp_diffuse_color.g; tl_diffuse_color.b = tmp_diffuse_color.b;
                yarn_type->color = tl_diffuse_color;
            }
        }
        if (is_param_valid(diffuse_color_on, i))
            yarn_type->color_enabled = get_bool(diffuse_color_on, i, rc);
        else if (is_param_valid(diffuse_color_on_float, i))
            yarn_type->color_enabled = get_bool(diffuse_color_on_float, i, rc);

    }

    tl_prepare(m_tl_wparams);

    pool.init(vray->getSequenceData().maxRenderThreads);
    return;
}

void BRDFThunderLoom::frameEnd(VRayRenderer *vray) {
    pool.freeMem();
    return;
}

BSDFSampler* BRDFThunderLoom::newBSDF(const VRayContext &rc, BSDFFlags flags) {
    BRDFThunderLoomSampler *bsdf = pool.newBRDF(rc);
    if (!bsdf) return NULL;
    //TODO pass along tl_wparams here!
    bsdf->init(rc, this->m_tl_wparams);
    return bsdf;
}

void BRDFThunderLoom::deleteBSDF(const VRayContext &rc, BSDFSampler *bsdf) {
    if (!bsdf) return;
    BRDFThunderLoomSampler *nbsdf = static_cast<BRDFThunderLoomSampler*>(bsdf);
    pool.deleteBRDF(rc, nbsdf);
}

using namespace VUtils;

// Implementation of Sampler.
void BRDFThunderLoomSampler::init(const VR::VRayContext &rc, tlWeaveParameters *tl_wparams) {
    orig_backside = rc.rayresult.realBack;

    normal = rc.rayresult.normal;    //smoothed normal
    gnormal = rc.rayresult.gnormal;  //geometric normal
    if (rc.rayresult.realBack) {
        normal = -normal;
        gnormal = -gnormal;
    }
    
    // Compute normal matrix, generates matrix for transforming from normal
    // space to world coordinates.
    if (rc.rayparams.viewDir*normal>0.0f) {
        makeNormalMatrix(gnormal, nm);
    } else {
        makeNormalMatrix(normal, nm);
    }
    inm = inverse(nm);

    // Use context rc to load and evalute tl. Cache common variables in
    // instance varaibles.
    m_tl_wparams = tl_wparams;
    if(!m_tl_wparams || m_tl_wparams->pattern ==0) { // Invalid pattern
        m_diffuse_color = Color(1.0f,1.0f,0.f);
        m_yarn_type = tl_default_yarn_type;
        m_yarn_type_id = 0;
        return;
    } 

    tlIntersectionData intersection_data;

    MappedSurface *mappedSurf=(MappedSurface*) GET_INTERFACE(rc.rayresult.sd, EXT_MAPPED_SURFACE);
    if (mappedSurf) {
        m_uv_tm = mappedSurf->getLocalUVWTransform(rc, -1);
        m_uv = m_uv_tm.offs;
    }
    intersection_data.uv_x = m_uv.x;
    intersection_data.uv_y = m_uv.y;
    intersection_data.wi_z = 1.f;
    
    intersection_data.context = (void *)&rc;

    tlPatternData pattern_data = tl_get_pattern_data(intersection_data, m_tl_wparams);

    m_yarn_type_id = pattern_data.yarn_type;
    m_yarn_type = m_tl_wparams->yarn_types[pattern_data.yarn_type];
    tlColor d = tl_eval_diffuse( intersection_data, pattern_data, m_tl_wparams);
    m_diffuse_color.r = d.r;
    m_diffuse_color.g = d.g;
    m_diffuse_color.b = d.b;

    return;
}

// FROM BRDFSampler

// Returns normal to use for diffuse lightinh
Vector BRDFThunderLoomSampler::getDiffuseNormal(const VR::VRayContext &rc) {
    return rc.rayresult.origNormal;
}

// Returns amount of light reflected diffusely by the surface.
Color BRDFThunderLoomSampler::getDiffuseColor(Color &lightColor) {
    Color ret = m_diffuse_color*lightColor;
    lightColor.makeZero();
    return ret;
}

// Total amount of light reflected by the surface.
Color BRDFThunderLoomSampler::getLightMult(Color &lightColor) {
    //TODO(Vidar):Make this work properly
    float s = m_yarn_type.specular_color.r;
    if(!m_yarn_type.specular_color_enabled
        && m_tl_wparams->num_yarn_types > 0){
        s = m_tl_wparams->yarn_types[0].specular_color.r;
    }
    Color ret = (m_diffuse_color + Color(s,s,s)) * lightColor;
    
    //lightColor is amount of light transmitted through the BRDF.
    lightColor.makeZero();
    return ret;
}

// Returns transparency of the BRDF at the given point.
Color BRDFThunderLoomSampler::getTransparency(const VR::VRayContext &rc) {
	return Color(0.f,0.f,0.f);
}

// Returns the amount of light transmitted from the given direction into the viewing direction.
Color BRDFThunderLoomSampler::eval(const VR::VRayContext &rc, const Vector &direction,
    Color &lightColor, Color &origLightColor, float probLight,
    int flags) {

    Color ret(0.f, 0.f, 0.f);

    float cs = direction * rc.rayresult.normal;
    cs = cs < 0.0f ? 0.0f : cs;

    if (flags & FBRDF_DIFFUSE) {
        float k = cs;
        // Apply combined sampling ONLY if GI is on which will pick up the rest
        // of the result.
        if (rc.rayparams.localRayType & RT_IS_GATHERING_POINT) {
            float probReflection=k;
            //Note(Vidar): Mutliple importance sampling factor
            k = getReflectionWeight(probLight, probReflection);
        }
        ret += m_diffuse_color * k;
    }
    if ((flags & FBRDF_SPECULAR) && ((rc.rayparams.rayType & RT_NOSPECULAR) == 0)) {
        Color reflect_color(0,0,0);
        //TODO(Vidar):Better importance sampling... Cosine weighted for now
        float probReflection=cs;

        // Evaluate specular reflection!
        if(!m_tl_wparams || m_tl_wparams->pattern == 0){ //Invalid pattern
            float s = 0.1f;
            reflect_color.r = s;
            reflect_color.g = s;
            reflect_color.b = s;
            return reflect_color;
        }
        tlIntersectionData intersection_data;
    
        // Convert the view and light directions to the correct coordinate 
        // system. We want these in uvw space, given by dp/du, dp/dv and normal.
        
        Vector viewDir, lightDir;
        viewDir = -rc.rayparams.getViewDir();
        lightDir = direction;

        Matrix iuv_tm = m_uv_tm.m; // Transform matrix from world space to uvw.
        iuv_tm.makeInverse0f();   // Inverse is world space to uvw.

        Vector u_vec = iuv_tm[0].getUnitVector();  // U dir
        Vector v_vec = iuv_tm[1].getUnitVector();  // V dir
        Vector n_vec = rc.rayresult.normal.getUnitVector();
        
        // Make sure these are orthonormal
        u_vec = crossf(v_vec, n_vec);
        v_vec = crossf(n_vec, u_vec);

        intersection_data.wi_x = dotf(lightDir, u_vec);
        intersection_data.wi_y = dotf(lightDir, v_vec);
        intersection_data.wi_z = dotf(lightDir, n_vec);

        intersection_data.wo_x = dotf(viewDir, u_vec);
        intersection_data.wo_y = dotf(viewDir, v_vec);
        intersection_data.wo_z = dotf(viewDir, n_vec);

        intersection_data.uv_x = m_uv.x;
        intersection_data.uv_y = m_uv.y;
        
        intersection_data.context = (void *)&rc;

        tlPatternData pattern_data = tl_get_pattern_data(intersection_data,
                m_tl_wparams);
        tlColor s = 
            tl_eval_specular(intersection_data, pattern_data, m_tl_wparams);

        reflect_color.r = s.r;
        reflect_color.g = s.g;
        reflect_color.b = s.b;

        //NOTE(Vidar): Multiple importance sampling factor
        float weight = getReflectionWeight(probLight,probReflection);
        ret += cs*reflect_color*weight;
    }

    ret *= lightColor;

    lightColor.makeZero();
    origLightColor.makeZero();

    return ret;
}

//NOTE(Vidar): Called whenever a reflection ray is to be calculated
void BRDFThunderLoomSampler::traceForward(VR::VRayContext &rc, int doDiffuse) {
	BRDFSampler::traceForward(rc, doDiffuse);
	if (doDiffuse) rc.mtlresult.color+=rc.evalDiffuse()*m_diffuse_color;
}

// Returns the number of desired samples for evaluating the BRDF.
int BRDFThunderLoomSampler::getNumSamples(const VR::VRayContext &rc, int doDiffuse) {
    if(rc.rayparams.currentPass==RPASS_GI ||
        (rc.rayparams.rayType & RT_LIGHT)!=0) {
        return 0;
    }
    //TODO(Vidar):Select using DMC sampler?
    return 8;
}

// Returns a new ray context for integrating the BRDF.
VR::VRayContext* BRDFThunderLoomSampler::getNewContext(const VR::VRayContext &rc, int &samplerID, int doDiffuse) {
	if (2==doDiffuse) return NULL;
    //TODO use specular strength to weight the samples.
    return NULL;
}

// This sets up the given context for tracing a ray for BRDF integration.
ValidType BRDFThunderLoomSampler::setupContext(const VR::VRayContext &rc, VR::VRayContext &nrc, float uc, int doDiffuse) {
    Vector dir = getDiffuseDir(uc, BRDFSampler::getDMCParam(nrc,1),nrc.rayparams.rayProbability);
    if (dir*gnormal<0.0f) return false;
    VR::getReflectDerivs(rc.rayparams.viewDir, dir, rc.rayparams.dDdx, rc.rayparams.dDdy, nrc.rayparams.dDdx, nrc.rayparams.dDdy);
    nrc.rayparams.tracedRay.dir=nrc.rayparams.viewDir=dir;
    return true;
}

RenderChannelsInfo* BRDFThunderLoomSampler::getRenderChannels(void) { return &RenderChannelsInfo::reflectChannels; }






// Called when a texture is applied to a float parameter.
float tl_eval_texmap_mono(void* texmap, void* context) {
    if (context) {
        VR::VRayContext *rc = (VR::VRayContext *)context;

        PluginBase* plug = static_cast<PluginBase *>(texmap);
        TextureInterface* tex = queryInterface<TextureInterface>(plug, EXT_TEXTURE);
        TextureFloatInterface* texf = queryInterface<TextureFloatInterface>(plug, EXT_TEXTURE_FLOAT);

        //TextureInterface* tex = static_cast<TextureInterface *>(GET_INTERFACE(plug, EXT_TEXTURE));
        //TextureFloatInterface* texf = static_cast<TextureFloatInterface *>(GET_INTERFACE(plug, EXT_TEXTURE_FLOAT));

        if (tex)
            return tex->getTexColor(*rc).color.r;
        else if (texf) {
            return texf->getTexFloat(*rc);
            //return 0.f;
        }
    }

    return 1.f;
}

struct tlShadeData: VR::VRayShadeData {
    tlShadeData(float u, float v) {
        this->u = u;
        this->v = v;
    }

    VR::Vector getUVWCoords(const VR::VRayContext &rc, int channel) {
        printf("get uvwcoords");
        return VR::Vector(this->u, this->v, 0.f);
    }
    
    void getUVWderivs(const VR::VRayContext &rc, int channel, VR::Vector derivs[2]) {
        derivs[0].makeZero();
        derivs[1].makeZero();
    }
    float u, v;
};

float tl_eval_texmap_mono_lookup(void *texmap, float u, float v, void *context)
{
    // Used to freely lookup in the texture map, using given u and v coords.
    // This is needed for propper yarnsize calculation when the yarnsize is
    // controlled via a texmap.
    //
    // V-Ray for Maya seems to only send arrays via textures. For Maya 
    // compatibly this function is also used to evaluate constant values
    // passed as float textures.
    
    //TODO(Peter): !!!! Get proper texture lookup working! I am having trouble
    // finding a solution to this.

    // Attempt 1
    // Create a VRayContext with theese coordinates and custom shadeData.
    // VRayContext rc;
    // tlShadeData sData = tlShadeData(u, v);
    // rc.rayresult.setShadeData(static_cast<VR::VRayShadeData*>(&sData));
    // float val = tl_eval_texmap_mono(texmap, &rc);
    // return val;
    // Does not seem to work.
    
    // Attempt 2
    // Check if texture is subclass of MappedTexture. 
    // See vray_plugins/textures/common/basetexture.h, The MappedTexture class
    // defines a getMappedFilteredColor virtual functions which I assume all 
    // textures of interest are derived from, eg TexChecker.
    // This does not use interfaces, and would not be so compatible with other
    // textures.
    // PluginBase* plug = static_cast<PluginBase *>(texmap);
    // //MappedTexture* mapped_tex = &texmap;
    // MappedTexture* mapped_tex = dynamic_cast<MappedTexture*>(plug);
    // if (mapped_tex) {
    //     printf("Is a mapped texture");
    //     VR::Vector uvw = VR::Vector(0.6,0.0,0.0);
    //     VR::Vector duvw = VR::Vector(0.0,0.0,0.0);
    //     mapped_tex->getMappedFilteredColor(rc, uvw, duvw);
    //     printf("Is a mapped texture");
    // }
    
    // Idea
    // Overload UVWGen interface of the texmap.
    
    return tl_eval_texmap_mono(texmap, context);
}

tlColor tl_eval_texmap_color(void *texmap, void *context)
{
    tlColor ret = {1.f,1.f,1.f};
    if (context) {
        PluginBase* plug = static_cast<PluginBase *>(texmap);
        TextureInterface* tex = queryInterface<TextureInterface>(plug, EXT_TEXTURE);
        VR::VRayContext *rc = (VR::VRayContext *)context;

        AColor col = tex->getTexColor(*rc);
        ret.r = col.color.r;
        ret.g = col.color.g;
        ret.b = col.color.b;
    }

    return ret;
}







//NOTE(Vidar):Return the correct BRDF depending on which side of the surface
// was hit...
BRDFSampler* BRDFThunderLoomSampler::getBRDF(BSDFSide side) {
    if (side==bsdfSide_front) {
        if (orig_backside) return NULL;
    } else {
        if (!orig_backside) return NULL;
    }
    return static_cast<BRDFSampler*>(this);
}


#define BRDFTest_PluginID PluginID(LARGE_CONST(2017041301))
// #ifdef __VRAY40__
// SIMPLE_PLUGIN_LIBRARY(BRDFTest_PluginID, "BRDFTest", "BRDFTestLibText", "a simple test plugin/lib", BRDFTest, BRDFTest_Params, EXT_BSDF);
// #else
SIMPLE_PLUGIN_LIBRARY(BRDFTest_PluginID, EXT_BSDF, "BRDFThunderLoom", "BRDFTestLibText", BRDFThunderLoom, BRDFThunderLoomParams);
// #endif

//PLUGIN_DESC(BRDFTest_PluginID, EXT_BSDF, "BRDFTest", BRDFTest, BRDFTest_Params);
