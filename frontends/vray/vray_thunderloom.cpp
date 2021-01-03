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

// NOTE: Some conditional changes in the code to deal with smaller api changes between vray 4 and 5.
#ifndef BUILD_VRAY5
#define BUILD_VRAY5 VRAY_DLL_VERSION_MAJOR >= 5
#endif

#define TL_THUNDERLOOM_IMPLEMENTATION
#include "thunderloom.h"
using namespace VUtils;
using namespace VR;

class BRDFThunderLoomSampler: public BRDFSampler, public BSDFSampler {
    int orig_backside;
    ShadeVec normal, gnormal;
    ShadeMatrix nm, inm;
    int nmInited;

    // Assigned in init()
    tlWeaveParameters* m_tl_wparams;
    ShadeVec m_uv;
    ShadeTransform m_uv_tm;
    ShadeCol m_diffuse_color;
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


struct BRDFThunderLoomParams: VRayParameterListDesc {
    BRDFThunderLoomParams(void) {
        addParamString("filepath", "", -1, "File path to pattern file");
        addParamFloat("uscale", 1.f, -1, "Scale factor for u coordinate");
        addParamFloat("vscale", 1.f, -1, "Scale factor for v coordinate");
        addParamFloat("uvrotation", 1.f, -1, "Rotation of the uv coordinates");
        
        // Yarn settings.
        // These are to be stored as lists in the .vrscene file. The index in
        // the list corresponds to the yarn_type.
        addParamPlugin("bend", EXT_TEXTURE_FLOAT, 0, "How much each visible yarn segment gets bent.");
        addParamPlugin("yarnsize", EXT_TEXTURE_FLOAT, 0, "Width of yarn.");
        addParamPlugin("twist", EXT_TEXTURE_FLOAT, 0, "How strongly to simulate the twisting nature of the yarn. Usually synthetic yarns, such as polyester, have less twist than organic yarns, such as cotton.");
        addParamPlugin("phase_alpha", EXT_TEXTURE_FLOAT, 0, "Alpha value. Constant term to the fiber scattering function.");
        addParamPlugin("phase_beta", EXT_TEXTURE_FLOAT, 0, "Beta value. Directionality intensity of the fiber scattering function.");
        addParamTexture("specular_color", Color(0.4f, 0.4f, 0.4f), 0, "Color of the specular reflections. This will implicitly affect the strength of the diffuse reflections.");
        addParamPlugin("specular_color_amount", EXT_TEXTURE_FLOAT, 0, "Factor to multiply specular color with.");
        addParamPlugin("specular_noise", EXT_TEXTURE_FLOAT, 0, "Noise on specular reflections.");
        addParamPlugin("highlight_width", EXT_TEXTURE_FLOAT, 0, "Width over which to average the specular reflections. Gives wider highlight streaks on the yarns.");
        addParamTexture("diffuse_color", Color(0.f, 0.3f, 0.f), 0, "Diffuse color.");
        addParamPlugin("diffuse_color_amount", EXT_TEXTURE_FLOAT, 0, "Factor to multiply diffuse color with.");
        
        // Stored as lists, just like above. These parameters allow us to 
        // specify what parameters we want to override, for a specific yarn.
        addParamBool("bend_on",                     false, 0, "");
        addParamBool("yarnsize_on",                 false, 0, "");
        addParamBool("twist_on",                    false, 0, "");
        addParamBool("phase_alpha_on",              false, 0, "");
        addParamBool("phase_beta_on",               false, 0, "");
        addParamBool("specular_color_on",           false, 0, "");
        addParamBool("specular_color_amount_on",    false, 0, "");
        addParamBool("specular_noise_on",           false, 0, "");
        addParamBool("highlight_width_on",          false, 0, "");
        addParamBool("diffuse_color_on",            false, 0, "");
        addParamBool("diffuse_color_amount_on",     false, 0, "");
    }
};

struct YarnCache {
	VRayPluginList bend;
	VRayPluginList yarnsize;
	VRayPluginList twist;
	VRayPluginList phase_alpha;
	VRayPluginList phase_beta;
	VRayPluginList specular_color_amount;
	VRayPluginList specular_noise;
	VRayPluginList highlight_width;
	VRayPluginList diffuse_color_amount;

	IntList bend_on;
	IntList yarnsize_on;
	IntList twist_on;
	IntList phase_alpha_on;
	IntList phase_beta_on;
	IntList specular_color_on;
	IntList specular_color_amount_on;
	IntList specular_noise_on;
	IntList highlight_width_on;
	IntList diffuse_color_on;
	IntList diffuse_color_amount_on;
};

struct YarnData {
	Table<TextureFloatInterface*> bend;
	Table<TextureFloatInterface*> yarnsize;
	Table<TextureFloatInterface*> twist;
	Table<TextureFloatInterface*> phase_alpha;
	Table<TextureFloatInterface*> phase_beta;
	Table<TextureFloatInterface*> specular_color_amount;
	Table<TextureFloatInterface*> specular_noise;
	Table<TextureFloatInterface*> highlight_width;
	Table<TextureFloatInterface*> diffuse_color_amount;
};

struct BRDFThunderLoom: VRayBSDF {
    BRDFThunderLoom(VRayPluginDesc *pluginDesc):VRayBSDF(pluginDesc) {
        paramList->setParamCache("filepath", &m_filepath);
        paramList->setParamCache("uscale", &m_uscale);
        paramList->setParamCache("vscale", &m_vscale);
        paramList->setParamCache("uvrotation", &m_uvrotation);
        paramList->setParamCache("bend", &yarnCache.bend);
        paramList->setParamCache("yarnsize", &yarnCache.yarnsize);
        paramList->setParamCache("twist", &yarnCache.twist);
        paramList->setParamCache("phase_alpha", &yarnCache.phase_alpha);
        paramList->setParamCache("phase_beta", &yarnCache.phase_beta);
        paramList->setParamCache("specular_color_amount", &yarnCache.specular_color_amount);
        paramList->setParamCache("specular_noise", &yarnCache.specular_noise);
        paramList->setParamCache("highlight_width", &yarnCache.highlight_width);
        paramList->setParamCache("diffuse_color_amount", &yarnCache.diffuse_color_amount);

		paramList->setParamCache("bend_on", &yarnCache.bend_on);
		paramList->setParamCache("yarnsize_on", &yarnCache.yarnsize_on);
		paramList->setParamCache("twist_on", &yarnCache.twist_on);
		paramList->setParamCache("phase_alpha_on", &yarnCache.phase_alpha_on);
		paramList->setParamCache("phase_beta_on", &yarnCache.phase_beta_on);
		paramList->setParamCache("specular_color_on", &yarnCache.specular_color_on);
		paramList->setParamCache("specular_color_amount_on", &yarnCache.specular_color_amount_on);
		paramList->setParamCache("specular_noise_on", &yarnCache.specular_noise_on);
		paramList->setParamCache("highlight_width_on", &yarnCache.highlight_width_on);
		paramList->setParamCache("diffuse_color_on", &yarnCache.diffuse_color_on);
		paramList->setParamCache("diffuse_color_amount_on", &yarnCache.diffuse_color_amount_on);
    }

	YarnCache yarnCache;
    tlWeaveParameters *m_tl_wparams;

    // From VRayBSDF
    void frameBegin(VRayRenderer *vray);
    void frameEnd(VRayRenderer *vray);

    // From BRDFInterface
    BSDFSampler *newBSDF(const VRayContext &rc, BSDFFlags flags);
    void deleteBSDF(const VRayContext &rc, BSDFSampler *bsdf);

    virtual PluginInterface* newInterface(InterfaceID id) {
        if (id==EXT_BSDF) return static_cast<PluginInterface*>((BSDFInterface*) this);
        return VRayPlugin::newInterface(id);
    }

	int getBSDFFlags(void) {
		return bsdfFlag_none;
	}

private:
    BRDFPool<BRDFThunderLoomSampler> pool;
    CharString m_filepath;
    float m_uscale, m_vscale, m_uvrotation;
    // Parameters
    PluginBase *baseBRDFParam;
};

//Param helpers
static FORCEINLINE int is_param_valid(VRayPluginParameter* param, int i) {
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

//Param helpers
static FORCEINLINE int is_texmap_valid(const Table<TextureFloatInterface*>& param, int i) {
    if (param.count() <= i)
        return 0;

    return param[i] != nullptr;
}

static FORCEINLINE int set_texparam(VRayPluginParameter* param, void **target, int i) {
    // returns true if it is a texture and sets target to the texture pointer.
	vassert(param != nullptr);
    VRayParameterType type = param->getType(i, 0);

    if (type == paramtype_object) {
		PluginBase* newPlug = param->getObject(i);
		*target = newPlug;

        return 1;
    }
    return 0;
}

void BRDFThunderLoom::frameBegin(VRayRenderer *vray) {
	if (!vray) return;
	VRayBSDF::frameBegin(vray);
#if BUILD_VRAY5
	//For vray5: 
    {
		const VUtils::AssetManager &assetMan=VUtils::getVRayAssetManager(vray);

		// Check for file, at path, then in VRAY_ASSETS_PATH and so forth.n, if exits and abort if not.
		if (!assetMan.checkAssetPath(m_filepath, true))
	        return;
    }
#else
    // For vray4
    {
		// Check for file, at path, then in VRAY_ASSETS_PATH and so forth.
		VUtils::checkAssetPath(m_filepath, vray->getSequenceData());
    
        // Check again, if exits and abort if not.
		VUtils::ProgressCallback *prog = vray->getSequenceData().progress;
		if (!VUtils::checkAssetPath(m_filepath, prog, true))
			return;
    }
#endif

    VUtils::ProgressCallback *prog = vray->getSequenceData().progress;

    // Load file and set the global settings.
    const char* filepath_str = (char*)m_filepath.ptr();
    const char *errors = 0;
    m_tl_wparams = tl_weave_pattern_from_file(filepath_str, &errors);
    if (errors) {
        prog->error("Error: ThunderLoom: %s", errors);
        return;
    }

    if (!m_tl_wparams) {
		return;
	}

	m_tl_wparams->uscale = m_uscale;
	m_tl_wparams->vscale = m_vscale;
	m_tl_wparams->uvrotation = m_uvrotation;
	m_tl_wparams->realworld_uv = 0;
	tl_prepare(m_tl_wparams);
    
    // Make new VRayContext
    VRayThreadData* vr_tdata = vray->getThreadData(0);
    VRayContext rc;
    rc.init(vr_tdata, vray);

    // Yarn type params
	YarnData yarnData;

    VRayPluginParameter* specular_color = this->getParameter("specular_color");
    VRayPluginParameter* diffuse_color = this->getParameter("diffuse_color");

#define TL_VRAY_INIT_PARAM(name) \
	for (int i = 0; i < yarnCache.name.count(); i++) { \
		yarnData.name += (TextureFloatInterface*) GET_INTERFACE(yarnCache.name[i], EXT_TEXTURE_FLOAT); \
	} \

#define TL_VRAY_FLOAT_PARAMS\
        TL_VRAY_INIT_PARAM(bend)\
        TL_VRAY_INIT_PARAM(yarnsize)\
        TL_VRAY_INIT_PARAM(twist)\
        TL_VRAY_INIT_PARAM(phase_alpha)\
        TL_VRAY_INIT_PARAM(phase_beta)\
        TL_VRAY_INIT_PARAM(highlight_width)\
        TL_VRAY_INIT_PARAM(specular_color_amount)\
        TL_VRAY_INIT_PARAM(specular_noise)\
        TL_VRAY_INIT_PARAM(diffuse_color_amount)\

	TL_VRAY_FLOAT_PARAMS

#undef TL_VRAY_FLOAT_PARAMS
#undef TL_VRAY_INIT_PARAM


    // Loop through yarn types and set parameters from list
    for (unsigned int i=0; i < m_tl_wparams->num_yarn_types; i++) {
        //get parameter from config string
		tlYarnType* yarn_type = &m_tl_wparams->yarn_types[i];

		//if (!set_texparam(name, &yarn_type->tl_name##_texmap, i))
#define TL_VRAY_SET_PARAM(name,tl_name) \
        if (is_texmap_valid(yarnData.name, i)) {\
                yarn_type->tl_name = yarnData.name[i]->getTexFloat(rc);\
        }\
        if (yarnCache.name##_on.count() > i) \
            yarn_type->tl_name##_enabled = yarnCache.name##_on[i]; \

#define TL_VRAY_FLOAT_PARAMS\
        TL_VRAY_SET_PARAM(bend, umax)\
        TL_VRAY_SET_PARAM(yarnsize, yarnsize)\
        TL_VRAY_SET_PARAM(twist, psi)\
        TL_VRAY_SET_PARAM(phase_alpha, alpha)\
        TL_VRAY_SET_PARAM(phase_beta, beta)\
        TL_VRAY_SET_PARAM(highlight_width, delta_x)\
        TL_VRAY_SET_PARAM(specular_color_amount, specular_amount)\
        TL_VRAY_SET_PARAM(specular_noise, specular_noise)\
        TL_VRAY_SET_PARAM(diffuse_color_amount, color_amount)\

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
        if (yarnCache.specular_color_on.count() > i)
            yarn_type->specular_color_enabled = yarnCache.specular_color_on[i];

        if (is_param_valid(diffuse_color, i)) {
            if (!set_texparam(diffuse_color, &yarn_type->color_texmap, i)) {
                Color tmp_diffuse_color = diffuse_color->getColor(i);
                tlColor tl_diffuse_color;
                tl_diffuse_color.r = tmp_diffuse_color.r; tl_diffuse_color.g = tmp_diffuse_color.g; tl_diffuse_color.b = tmp_diffuse_color.b;
                yarn_type->color = tl_diffuse_color;
            }
        }
		if (yarnCache.diffuse_color_on.count() > i)
			yarn_type->color_enabled = yarnCache.diffuse_color_on[i];

    }

    tl_prepare(m_tl_wparams);

    pool.init(vray->getSequenceData().maxRenderThreads);
    return;
}

void BRDFThunderLoom::frameEnd(VRayRenderer *vray) {
    VRayBSDF::frameEnd(vray);
    pool.freeMem();
}

BSDFSampler* BRDFThunderLoom::newBSDF(const VRayContext &rc, BSDFFlags flags) {
    BRDFThunderLoomSampler *bsdf = pool.newBRDF(rc);
    if (!bsdf) return NULL;
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
//NOTE(Vidar): This function is called for each intersection with the material
// From this point, several rays can be fired in different directions, each
// one calling eval(). In this function we can do all the work that is common
// throughout all directions, such as computing the diffuse color.
void BRDFThunderLoomSampler::init(const VR::VRayContext &rc, tlWeaveParameters *tl_wparams) {
    orig_backside = rc.rayresult.realBack;

    normal = rc.rayresult.normal;    //smoothed normal
    gnormal = rc.rayresult.gnormal;  //geometric normal
    if (rc.rayresult.realBack) {
        normal = -normal;
        gnormal = -gnormal;
    }
    
    nmInited = 0;

    // Use context rc to load and evalute tl. Cache common variables in
    // instance varaibles.
    m_tl_wparams = tl_wparams;
    if(!m_tl_wparams || m_tl_wparams->pattern ==0) { // Invalid pattern
        m_diffuse_color = ShadeCol(1.0f,1.0f,0.f);
        m_yarn_type = tl_default_yarn_type;
        m_yarn_type_id = 0;
        return;
    }

    tlIntersectionData intersection_data;

    MappedSurface *mappedSurf=(MappedSurface*) GET_INTERFACE(rc.rayresult.sd, EXT_MAPPED_SURFACE);
    if (mappedSurf) {
#if BUILD_VRAY5
        mappedSurf->getLocalUVWTransform(rc, -1, m_uv_tm, uvwFlags_default); // for vray5
#else
        mappedSurf->getLocalUVWTransform(rc, -1, m_uv_tm); // for vray4
#endif
        m_uv = m_uv_tm.offs;
    }
    intersection_data.uv_x = m_uv.x();
    intersection_data.uv_y = m_uv.y();
    intersection_data.wi_z = 1.f;
    
    intersection_data.context = (void *)&rc;

    tlPatternData pattern_data = tl_get_pattern_data(intersection_data, m_tl_wparams);

    m_yarn_type_id = pattern_data.yarn_type;
    m_yarn_type = m_tl_wparams->yarn_types[pattern_data.yarn_type];
    tlColor d = tl_eval_diffuse( intersection_data, pattern_data, m_tl_wparams);
	m_diffuse_color.set(d.r, d.g, d.b);

    return;
}

// FROM BRDFSampler

//NOTE(Vidar): Return the normal used for diffuse shading. If we do bump mapping
// on the diffuse color, we will have to return the modified normal here...
VUtils::ShadeVec BRDFThunderLoomSampler::getDiffuseNormal(const VR::VRayContext &rc)
{
    return rc.rayresult.origNormal;
}

// Returns amount of light reflected diffusely by the surface.
VUtils::ShadeCol BRDFThunderLoomSampler::getDiffuseColor(VUtils::ShadeCol &lightColor) {
    VUtils::ShadeCol ret = m_diffuse_color*lightColor;
	lightColor.makeZero();
    return ret;
}

// Total amount of light reflected by the surface.
ShadeCol BRDFThunderLoomSampler::getLightMult(ShadeCol &lightColor) {
    tlColor s = m_yarn_type.specular_color;
    if(!m_yarn_type.specular_color_enabled
        && m_tl_wparams->num_yarn_types > 0){
        s = m_tl_wparams->yarn_types[0].specular_color;
    }
    ShadeCol ret = (m_diffuse_color + ShadeCol(s.r,s.g,s.b)) * lightColor;
    lightColor.makeZero();
    return ret;
}

// Returns transparency of the BRDF at the given point.
ShadeCol BRDFThunderLoomSampler::getTransparency(const VR::VRayContext &rc) {
	return ShadeCol(0.f);
}

// Returns the amount of light transmitted from the given direction into the viewing direction.
ShadeCol BRDFThunderLoomSampler::eval( const VRayContext &rc, const ShadeVec &direction, ShadeCol &shadowedLight,
	ShadeCol &origLight, float probLight,int flags)
{

    ShadeCol ret(0.f, 0.f, 0.f);

    const ShadeVec &vecN = normal;
    const ShadeVec &vecL = direction;
    float NL = dotf(vecN, vecL);

    NL = VR::clamp(NL, -1.0f, 1.0f); // make sure acosf won't return NaN

    float cs = NL;
    if (cs < 0.0f)
        cs = 0.0f;

    float k = cs;

    if (flags & FBRDF_DIFFUSE) {
        // Apply combined sampling ONLY if GI is on which will pick up the rest
        // of the result.
        if (rc.rayparams.localRayType & RT_IS_GATHERING_POINT) {
            float probReflection=k;
            //Note(Vidar): Mutliple importance sampling factor
            k = cs*getReflectionWeight(probLight, probReflection);
        }
        ret += m_diffuse_color * k;
    }
    if ((flags & FBRDF_SPECULAR) && ((rc.rayparams.rayType & RT_NOSPECULAR) == 0)) {
        ShadeCol reflect_color(0.f);
        //TODO(Vidar):Better importance sampling... Cosine weighted for now
        float probReflection=cs;

        // Evaluate specular reflection!
        if(!m_tl_wparams || m_tl_wparams->pattern == 0){ //Invalid pattern
            float s = 0.1f;
			reflect_color.set(s, s, s);
            return reflect_color;
        }
        tlIntersectionData intersection_data;
    
        // Convert the view and light directions to the correct coordinate 
        // system. We want these in uvw space, given by dp/du, dp/dv and normal.
        
        ShadeVec viewDir, lightDir;
        viewDir = -rc.rayparams.getViewDir();
        lightDir = direction;

        ShadeMatrix iuv_tm = m_uv_tm.m; // Transform matrix from world space to uvw.
        iuv_tm.makeInverse0f();   // Inverse is world space to uvw.

        ShadeVec u_vec = iuv_tm[0].getUnitVector();  // U dir
        ShadeVec v_vec = iuv_tm[1].getUnitVector();  // V dir
        ShadeVec n_vec = rc.rayresult.normal.getUnitVector();
        
        // Make sure these are orthonormal
        u_vec = crossf(v_vec, n_vec);
        v_vec = crossf(n_vec, u_vec);

        intersection_data.wi_x = dotf(lightDir, u_vec);
        intersection_data.wi_y = dotf(lightDir, v_vec);
        intersection_data.wi_z = dotf(lightDir, n_vec);

        intersection_data.wo_x = dotf(viewDir, u_vec);
        intersection_data.wo_y = dotf(viewDir, v_vec);
        intersection_data.wo_z = dotf(viewDir, n_vec);

        intersection_data.uv_x = m_uv.x();
        intersection_data.uv_y = m_uv.y();
        
        intersection_data.context = (void *)&rc;

        tlPatternData pattern_data = tl_get_pattern_data(intersection_data,
                m_tl_wparams);
        tlColor s = 
            tl_eval_specular(intersection_data, pattern_data, m_tl_wparams);

        reflect_color.set(s.r, s.g, s.b);

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
void BRDFThunderLoomSampler::traceForward(VR::VRayContext &rc, int doDiffuse) {
	BRDFSampler::traceForward(rc, doDiffuse);
	if (doDiffuse) rc.mtlresult.color+=rc.evalDiffuse()*m_diffuse_color;
    Fragment *f=rc.mtlresult.fragment;
    if(f){
        ShadeCol raw_gi=rc.evalDiffuse();
        ShadeCol diffuse_gi=raw_gi*m_diffuse_color;
        f->setChannelDataByAlias(REG_CHAN_VFB_RAWGI,&raw_gi);
        f->setChannelDataByAlias(REG_CHAN_VFB_RAWTOTALLIGHT,&raw_gi);
        f->setChannelDataByAlias(REG_CHAN_VFB_GI,&diffuse_gi);
        f->setChannelDataByAlias(REG_CHAN_VFB_TOTALLIGHT,&diffuse_gi);
        f->setChannelDataByAlias(REG_CHAN_VFB_DIFFUSE,&raw_gi);
    }
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
	//TODO(Vidar):Why does this differ from the 3ds max version?
    return NULL;
}

// This sets up the given context for tracing a ray for BRDF integration.
ValidType BRDFThunderLoomSampler::setupContext(const VR::VRayContext &rc, VR::VRayContext &nrc, float uc, int doDiffuse) {
    /*Vector dir = getDiffuseDir(uc, BRDFSampler::getDMCParam(nrc,1),nrc.rayparams.rayProbability);
    if (dir*gnormal<0.0f) return false;
    VR::getReflectDerivs(rc.rayparams.viewDir, dir, rc.rayparams.dDdx, rc.rayparams.dDdy, nrc.rayparams.dDdx, nrc.rayparams.dDdy);
    nrc.rayparams.tracedRay.dir=nrc.rayparams.viewDir=dir;
    return true;*/

    ShadeVec dir;
    if (rc.rayresult.getSurfaceHit()!=2) {
        if (!nmInited) {
            makeNormalMatrix(rc.rayresult.normal, nm);
            nmInited=true;
        }
        dir=nm*VR::getDiffuseDir3f(uc, getDMCParam(nrc, 1), nrc.rayparams.rayProbability);
    } else {
        dir=VR::getSphereDir3f(2.0f*(uc-0.5f), getDMCParam(nrc, 1));
        nrc.rayparams.rayProbability=0.5f;
    }

    if (dotf(dir, rc.rayresult.gnormal)>-1e-3f || rc.rayresult.getSurfaceHit()==2) {
        VR::getReflectDerivs(rc.rayparams.viewDir, dir, rc.rayparams.dDdx, rc.rayparams.dDdy, nrc.rayparams.dDdx, nrc.rayparams.dDdy);
        int rDistOn=rc.vray->getSequenceData().params.gi.rayDistanceOn;
        float rDist=rDistOn?rc.vray->getSequenceData().params.gi.rayDistance:LARGE_FLOAT;
        VR::setTracedDir(nrc, dir, 0.0f, rDist, true);
        return true;
    }

    return false;
}

RenderChannelsInfo* BRDFThunderLoomSampler::getRenderChannels(void) { return &RenderChannelsInfo::reflectChannels; }


// Following is used to to allow custom uv lookups in texture maps, see...
// https://forums.chaosgroup.com/forum/v-ray-for-maya-forums/v-ray-for-maya-sdk/76255-texture-lookup-with-uv-on-non-bitmap-textures
struct MyShadeData: public VRayShadeData, public MappedSurface {
    VRayContext *rcc;
    VRayShadeData *oldShadeData;
    ShadeVec uvw;

    MyShadeData(const VRayContext &rc, const ShadeVec &uvw):uvw(uvw) {
        oldShadeData=rc.rayresult.sd;
        rcc=const_cast<VRayContext*>(&rc);
        rcc->rayresult.sd=static_cast<VRayShadeData*>(this);
    }

    ~MyShadeData() {
        rcc->rayresult.sd=oldShadeData;
    }

    PluginBase* getPlugin(void) VRAY_OVERRIDE { return (PluginBase*) this; }
    PluginInterface* newInterface(InterfaceID id) VRAY_OVERRIDE {
        if (id==EXT_MAPPED_SURFACE) return static_cast<MappedSurface*>(this);
        return oldShadeData? oldShadeData->newInterface(id) : VRayShadeData::newInterface(id);
    }

    // From MappedSurface
#if BUILD_VRAY5
	void getLocalUVWTransform(const VRayContext &rc, int channel, ShadeTransform &result, const UVWFlags uvwFlags) VRAY_OVERRIDE // for vray5
#else
	void getLocalUVWTransform(const VRayContext &rc, int channel, ShadeTransform &result) VRAY_OVERRIDE // for vray4
#endif
	{
        // Note that here we initialize the matrix to zero. This will effectively kill any
        // texture filtering. If you want texture filtering, you will need to come up with
        // a suitable way to initialize the Matrix portion of the transform to a matrix that
        // converts world-space changes in the shading point to changes in UVWs.
        Transform res(0);
		result.makeZero();
		result.offs = uvw;
    }
};

// Called when a texture is applied to a float parameter.
float tl_eval_texmap_mono(void* texmap, void* context) {
    if (context) {
        VR::VRayContext *rc = (VR::VRayContext *)context;

        PluginBase* plug = static_cast<PluginBase *>(texmap);
        TextureInterface* tex = queryInterface<TextureInterface>(plug, EXT_TEXTURE);
        TextureFloatInterface* texf = queryInterface<TextureFloatInterface>(plug, EXT_TEXTURE_FLOAT);

        if (tex)
            return tex->getTexColor(*rc).color.r;
        else if (texf) {
            return texf->getTexFloat(*rc);
        }
    }

    return 1.f;
}

float tl_eval_texmap_mono_lookup(void *texmap, float u, float v, void *context)
{
    // Used to freely lookup in the texture map, using given u and v coords.
    // This is needed for propper yarnsize calculation when the yarnsize is
    // controlled via a texmap.
    //
    // V-Ray for Maya seems to only send arrays via textures. For Maya 
    // compatibly this function is also used to evaluate constant values
    // passed as float textures.

    if (context) {
        VRayContext *rc = (VRayContext *)context;

        PluginBase* plug = static_cast<PluginBase *>(texmap);
        TextureInterface* tex = queryInterface<TextureInterface>(plug, EXT_TEXTURE);
        TextureFloatInterface* texf = queryInterface<TextureFloatInterface>(plug, EXT_TEXTURE_FLOAT);
    
        ShadeVec uvw(u, v, 0.f);
		//TODO(Vidar):Reenable this?
        //MyShadeData myShadeData(*rc, uvw);

        if (tex) {
            return tex->getTexColor(*rc).color.r; 
        } else if (texf) {
            return texf->getTexFloat(*rc);
        }
    }
    return 1.f;
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


#define BRDFThunderLoom_PluginID PluginID(LARGE_CONST(2021010201))
SIMPLE_PLUGIN_LIBRARY(BRDFThunderLoom_PluginID, "BRDFThunderLoomSource", "BRDFThunderLoomSource", "Cloth and fabric BRDF from source code", BRDFThunderLoom, BRDFThunderLoomParams, EXT_BSDF);
