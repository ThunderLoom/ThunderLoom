// Disable MSVC warnings for external headers.

#include "vray_thunderloom.h"

using namespace VUtils;
using namespace VR;

BRDFThunderLoomParamsUpdated::BRDFThunderLoomParamsUpdated(void) {
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

BRDFThunderLoomUpdated::BRDFThunderLoomUpdated(VRayPluginDesc *pluginDesc):VRayBSDF(pluginDesc) {
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

//Param helpers
static FORCEINLINE int is_texmap_valid(const Table<TextureFloatInterface*>& param, int i) {
    if (param.count() <= i)
        return 0;

    return param[i] != nullptr;
}

void BRDFThunderLoomUpdated::frameBegin(VRayRenderer *vray) {
    if (!vray) return;
    VRayBSDF::frameBegin(vray);

    //Check for file, at path, then in VRAY_ASSETS_PATH and so forth.
    VUtils::checkAssetPath(m_filepath, vray->getSequenceData());
    
    //Check again, if exits and abort if not.
    VUtils::ProgressCallback *prog = vray->getSequenceData().progress;
    if (!VUtils::checkAssetPath(m_filepath, prog, true))
        return;


    // Load file and set the global settings.
	// Now we don't need to get the params because they are already in the cashe 
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

	// Same here for the params getting them from the cashe
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

	// For these VRay doesn't support caching still
    VRayPluginParameter* specular_color = this->getParameter("specular_color");
    VRayPluginParameter* diffuse_color = this->getParameter("diffuse_color");

	// Here we get them from the cache and save them as TextureFloatInterfaces in the tables
	// So we can use them more easily in the trasnsforming to TL params below
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

void BRDFThunderLoomUpdated::frameEnd(VRayRenderer *vray) {
    VRayBSDF::frameEnd(vray);
    pool.freeMem();
}

BSDFSampler* BRDFThunderLoomUpdated::newBSDF(const VRayContext &rc, BSDFFlags flags) {
    BRDFThunderLoomSampler *bsdf = pool.newBRDF(rc);
    if (!bsdf) return NULL;
    bsdf->init(rc, this->m_tl_wparams);
    return bsdf;
}

void BRDFThunderLoomUpdated::deleteBSDF(const VRayContext &rc, BSDFSampler *bsdf) {
    if (!bsdf) return;
    BRDFThunderLoomSampler *nbsdf = static_cast<BRDFThunderLoomSampler*>(bsdf);
    pool.deleteBRDF(rc, nbsdf);
}
