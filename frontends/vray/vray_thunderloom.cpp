// test plugin

#define TL_THUNDERLOOM_IMPLEMENTATION
#include "vray_thunderloom.h"
using namespace VUtils;
using namespace VR;


struct BRDFThunderLoomParams: VRayParameterListDesc {
    BRDFThunderLoomParams(void) {
        addParamString("filepath", "", -1, "File path to pattern file");
        addParamFloat("uscale", 1.f, -1, "Scale factor for u coordinate");
        addParamFloat("vscale", 1.f, -1, "Scale factor for v coordinate");
        addParamFloat("uvrotation", 1.f, -1, "Rotation of the uv coordinates");
        
        // Yarn settings.
        // These are to be stored as lists in the .vrscene file. The index in
        // the list corresponds to the yarn_type.
        addParamTextureFloat("bend", 0.5f, -1, "How much each visible yarn segment gets bent.");
        addParamTextureFloat("yarnsize", 1.0f, -1, "Width of yarn.");
        addParamTextureFloat("twist", 0.5f, -1, "How strongly to simulate the twisting nature of the yarn. Usually synthetic yarns, such as polyester, have less twist than organic yarns, such as cotton.");
        addParamTextureFloat("phase_alpha", 0.05f, -1, "Alpha value. Constant term to the fiber scattering function.");
        addParamTextureFloat("phase_beta", 4.f, -1, "Beta value. Directionality intensity of the fiber scattering function.");
        addParamTexture("specular_color", AColor(0.4f, 0.4f, 0.4f, 1.0f), -1, "Color of the specular reflections. This will implicitly affect the strength of the diffuse reflections.");
        addParamTextureFloat("specular_color_amount", 1.0f, -1, "Factor to multiply specular color with.");
        addParamTextureFloat("specular_noise", 0.4f, -1, "Noise on specular reflections.");
        addParamTextureFloat("highlight_width", 0.4f, -1, "Width over which to average the specular reflections. Gives wider highlight streaks on the yarns.");
        addParamTexture("diffuse_color", AColor(0.f, 0.3f, 0.f, 1.0f), -1, "Diffuse color.");
        addParamTextureFloat("diffuse_color_amount", 1.0f, -1, "Factor to multiply diffuse color with.");
        
        // Stored as lists, just like above. These parameters allow us to 
        // specify what parameters we want to override, for a specific yarn.
        addParamBool("bend_on",                     false, -1, "");
        addParamBool("yarnsize_on",                 false, -1, "");
        addParamBool("twist_on",                    false, -1, "");
        addParamBool("phase_alpha_on",              false, -1, "");
        addParamBool("phase_beta_on",               false, -1, "");
        addParamBool("specular_color_on",           false, -1, "");
        addParamBool("specular_color_amount_on",    false, -1, "");
        addParamBool("specular_noise_on",           false, -1, "");
        addParamBool("highlight_width_on",          false, -1, "");
        addParamBool("diffuse_color_on",            false, -1, "");
        addParamBool("diffuse_color_amount_on",     false, -1, "");

        // MAYA FIX
        // It seems that only float params can be retrieved from Maya into the 
        // vrscene file. And only as a texture if the floats are put into an 
        // array. We allow the _on params to be specified as a float aswell.
        addParamTextureFloat("bend_on_float",                  false, -1, "");
        addParamTextureFloat("yarnsize_on_float",              false, -1, "");
        addParamTextureFloat("twist_on_float",                 false, -1, "");
        addParamTextureFloat("phase_alpha_on_float",           false, -1, "");
        addParamTextureFloat("phase_beta_on_float",            false, -1, "");
        addParamTextureFloat("specular_color_on_float",        false, -1, "");
        addParamTextureFloat("specular_color_amount_on_float", false, -1, "");
        addParamTextureFloat("specular_noise_on_float",        false, -1, "");
        addParamTextureFloat("highlight_width_on_float",       false, -1, "");
        addParamTextureFloat("diffuse_color_on_float",         false, -1, "");
        addParamTextureFloat("diffuse_color_amount_on_float",  false, -1, "");
        
        addParamFloat("bends2", 0.5f, -1, "");
    }
};

struct BRDFThunderLoom: VRayBSDF {
    BRDFThunderLoom(VRayPluginDesc *pluginDesc):VRayBSDF(pluginDesc) {
        //bends("bend_vals");
        paramList->setParamCache("filepath", &m_filepath);
        paramList->setParamCache("uscale", &m_uscale);
        paramList->setParamCache("vscale", &m_vscale);
        paramList->setParamCache("uvrotation", &m_uvrotation);
        
    }

    tlWeaveParameters *m_tl_wparams;

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


FORCEINLINE int get_bool(VRayPluginParameter * param, int i, VRayContext& rc) {
    if (param->getType(i,0) == VRayParameterType::paramtype_bool && param->getBool(i))
      return 1;
    
    // For maya support ([param]_on_float), float arrays are sent as textures.
    void* tex;
    if (set_texparam(param, &tex, i)) {
        // param was texture and tex is a pointer to it.
        // Get texture value
        if (tl_eval_texmap_mono_lookup(tex, 0.f, 0.f, &rc))
            return 1;
    }

    return 0;
}

void BRDFThunderLoom::frameBegin(VRayRenderer *vray) {
    if (!vray) return;
	// Calling parent frameBegin so the caching of params can work
    VRayBSDF::frameBegin(vray);

    //Check for file, at path, then in VRAY_ASSETS_PATH and so forth.
    VUtils::checkAssetPath(m_filepath, vray->getSequenceData());
	
    //Check again, if exits and abort if not.
    VUtils::ProgressCallback *prog = vray->getSequenceData().progress;
    if (!VUtils::checkAssetPath(m_filepath, prog, true))
        return;


    // Load file and set the global settings.
    const char* filepath_str = (char*)m_filepath.ptr();
    const char *errors = 0;
    m_tl_wparams = tl_weave_pattern_from_file(filepath_str, &errors);
    if (errors) {
        prog->error("Error: ThunderLoom: %s", errors);
        return; //Abort a little nicer
    }

    if (!m_tl_wparams) {
        return;
    }

    if (m_tl_wparams) {
        m_tl_wparams->uscale = 1.f;
        m_tl_wparams->vscale = 1.f;
        m_tl_wparams->uvrotation = 0.f;
        m_tl_wparams->realworld_uv = 0;
        tl_prepare(m_tl_wparams);
    }
    
    // Make new VRayContext
    VRayThreadData* vr_tdata = vray->getThreadData(0);
    VRayContext rc;
    rc.init(vr_tdata, vray);
	rc.clear();
    
	m_tl_wparams->uscale = m_uscale;
	m_tl_wparams->vscale = m_vscale;
	m_tl_wparams->uvrotation = m_uvrotation;

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
    
    VRayPluginParameter* phase_alpha = this->getParameter("phase_alpha");
    VRayPluginParameter* phase_alpha_on = this->getParameter("phase_alpha_on");
    VRayPluginParameter* phase_alpha_on_float = this->getParameter("phase_alpha_on_float");

    VRayPluginParameter* phase_beta = this->getParameter("phase_beta");
    VRayPluginParameter* phase_beta_on = this->getParameter("phase_beta_on");
    VRayPluginParameter* phase_beta_on_float = this->getParameter("phase_beta_on_float");

    VRayPluginParameter* specular_color = this->getParameter("specular_color");
    VRayPluginParameter* specular_color_on = this->getParameter("specular_color_on");
    VRayPluginParameter* specular_color_on_float = this->getParameter("specular_color_on_float");
    
    VRayPluginParameter* specular_color_amount = this->getParameter("specular_color_amount");
    VRayPluginParameter* specular_color_amount_on = this->getParameter("specular_color_amount_on");
    VRayPluginParameter* specular_color_amount_on_float = this->getParameter("specular_color_amount_on_float");

    VRayPluginParameter* specular_noise = this->getParameter("specular_noise");
    VRayPluginParameter* specular_noise_on = this->getParameter("specular_noise_on");
    VRayPluginParameter* specular_noise_on_float = this->getParameter("specular_noise_on_float");

    VRayPluginParameter* highlight_width = this->getParameter("highlight_width");
    VRayPluginParameter* highlight_width_on = this->getParameter("highlight_width_on");
    VRayPluginParameter* highlight_width_on_float = this->getParameter("highlight_width_on_float");
    
    VRayPluginParameter* diffuse_color = this->getParameter("diffuse_color");
    VRayPluginParameter* diffuse_color_on = this->getParameter("diffuse_color_on");
    VRayPluginParameter* diffuse_color_on_float = this->getParameter("diffuse_color_on_float");
    
    VRayPluginParameter* diffuse_color_amount = this->getParameter("diffuse_color_amount");
    VRayPluginParameter* diffuse_color_amount_on = this->getParameter("diffuse_color_amount_on");
    VRayPluginParameter* diffuse_color_amount_on_float = this->getParameter("diffuse_color_amount_on_float");


    // Loop through yarn types and set parameters from list
    for (unsigned int i=0; i < m_tl_wparams->num_yarn_types; i++) {
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
        if (is_param_valid(specular_color_on, i))
            yarn_type->specular_color_enabled = get_bool(specular_color_on, i, rc);
        else if (is_param_valid(specular_color_on_float, i))
            yarn_type->specular_color_enabled = get_bool(specular_color_on_float, i, rc);

        if (is_param_valid(diffuse_color, i)) {
            if (!set_texparam(diffuse_color, &yarn_type->color_texmap, i)) {
                AColor tmp_diffuse_color = diffuse_color->getAColor(i);
                tlColor tl_diffuse_color;
                tl_diffuse_color.r = tmp_diffuse_color.color.r;
                tl_diffuse_color.g = tmp_diffuse_color.color.g;
                tl_diffuse_color.b = tmp_diffuse_color.color.b;
                tl_diffuse_color.a = tmp_diffuse_color.alpha;
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
        m_diffuse_color_alpha = 1.f;
        m_yarn_type = tl_default_yarn_type;
        m_yarn_type_id = 0;
        return;
    } 

    tlIntersectionData intersection_data;

    MappedSurface *mappedSurf=(MappedSurface*) GET_INTERFACE(rc.rayresult.sd, EXT_MAPPED_SURFACE);
    if (mappedSurf) {
        mappedSurf->getLocalUVWTransform(rc, -1, m_uv_tm );
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
	m_diffuse_color_alpha = d.a;

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
	return ShadeCol(1.f - m_diffuse_color_alpha);

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
	void getLocalUVWTransform(const VRayContext &rc, int channel, ShadeTransform &result) VRAY_OVERRIDE
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


#define BRDFThunderLoom_PluginID PluginID(LARGE_CONST(2017041301))

// With this explicit library descriptor declaration we are making sure that
// the correct ThunderLoom version is loaded for the correct VRay version

static BRDFThunderLoomParams pluginParams;
static BRDFThunderLoomParamsUpdated pluginParamsUpdated;

static struct UserPluginDesc : VRayPluginDesc {
    UserPluginDesc(void)
        : VRayPluginDesc(getParams()) {}

    PluginID getPluginID(void) { return BRDFThunderLoom_PluginID; }

    Plugin* newPlugin(PluginHost* host) { 
		// The only differences than the macro variant are in here and in the delete below
		if (shouldBeUpdatedVersion()) return new BRDFThunderLoomUpdated(this);
		else return new BRDFThunderLoom(this); 
	}
    void deletePlugin(Plugin* obj) {
		if (shouldBeUpdatedVersion()) delete (BRDFThunderLoomUpdated*)obj;
		else delete (BRDFThunderLoom*)obj;
	}

    bool supportsInterface(InterfaceID id) { return _supportsInterface(id, EXT_VRAY_PLUGIN, EXT_BSDF, +0, 0); }
    VRAY3_CONST_COMPAT tchar* getName(void) { return "BRDFThunderLoom"; }
    const tchar* getCopyright(void) { return NULL; }
    const tchar* getDescription() const { return "Cloth and fabric BRDF"; }
    const tchar* getDeprecation() const { return NULL; }
    const tchar* getAliases() const { return NULL; }
private:
	static FORCEINLINE bool shouldBeUpdatedVersion() {
		return getVRayRevision() >= 0x42010;
	}

	static FORCEINLINE VRayParameterListDesc* getParams() {
		if (shouldBeUpdatedVersion()) return &pluginParamsUpdated;
		else return &pluginParams;
	}

    bool _supportsInterface(InterfaceID id, ...)
    {
        va_list vargs;
        va_start(vargs, id);
        unsigned long long i;
        while ((i = va_arg(vargs, unsigned long long))) {
            if (id == i) {
                va_end(vargs);
                return true;
            }
        }
        va_end(vargs);
        return false;
    }
} pluginDesc;
struct LibDesc : PluginLibDesc {
    VRAY3_CONST_COMPAT tchar* getName(void) { return "BRDFThunderLoom"; }
    VRAY3_CONST_COMPAT tchar* getText(void) { return "BRDFThunderLoom"; }
    int enumPluginDescs(EnumPluginDescCallback* cb)
    {
        if (cb)
            cb->process(&pluginDesc);
        return 1;
    }
} libDesc;
EXPORT_PLUGIN_LIBRARY(libDesc);
;
