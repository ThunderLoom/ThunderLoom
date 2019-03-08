// test plugin

// Disable MSVC warnings for external headers.
#pragma warning( push )
#pragma warning( disable : 4251)
#pragma warning( disable : 4996 )
#include "vrayplugins.h"
#include "vrayinterface.h"
#include "vrayrenderer.h"
#include "brdfs.h"
#include "vraytexutils.h"
#include "defparams.h"
#pragma warning( pop ) 

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
#define TL_FLOAT_PARAM(name) addParamTextureFloat(#name , 1.f, -1, "TODO(Vidar):Specify defaults and descriptions");
#define TL_COLOR_PARAM(name) addParamTexture(#name , Color(1.f,1.f,1.f), -1, "TODO(Vidar):Specify defaults and descriptions");
		TL_YARN_PARAMETERS
#undef TL_FLOAT_PARAM
#undef TL_COLOR_PARAM
        //addParamTexture("diffuse_color", Color(0.f, 0.3f, 0.f), -1, "Diffuse color.");
        //addParamTextureFloat("diffuse_color_amount", 1.0f, -1, "Factor to multiply diffuse color with.");
        
        // Stored as lists, just like above. These parameters allow us to 
        // specify what parameters we want to override, for a specific yarn.
#define TL_FLOAT_PARAM(name) addParamBool(#name "_on", false, -1, "");
#define TL_COLOR_PARAM(name) addParamBool(#name "_on", false, -1, "");
		TL_YARN_PARAMETERS
#undef TL_FLOAT_PARAM
#undef TL_COLOR_PARAM
        //addParamBool("diffuse_color_amount_on",     false, -1, "");

        // MAYA FIX
        // It seems that only float params can be retrieved from Maya into the 
        // vrscene file. And only as a texture if the floats are put into an 
        // array. We allow the _on params to be specified as a float aswell.
#define TL_FLOAT_PARAM(name) addParamTextureFloat(#name "_on_float", false, -1, "");
#define TL_COLOR_PARAM(name) addParamTextureFloat(#name "_on_float", false, -1, "");
		TL_YARN_PARAMETERS
#undef TL_FLOAT_PARAM
#undef TL_COLOR_PARAM
        
        addParamFloat("bends2", 0.5f, -1, "");
    }
};

//Struct for intermediatly storing yarn parameters.
typedef struct {
#define TL_FLOAT_PARAM(name) float name;
#define TL_COLOR_PARAM(name) Color name;
		TL_YARN_PARAMETERS
#undef TL_FLOAT_PARAM
#undef TL_COLOR_PARAM
} tlIntermediateYrnParam;

struct BRDFThunderLoom: VRayBSDF {
    BRDFThunderLoom(VRayPluginDesc *pluginDesc):VRayBSDF(pluginDesc), bends2() {
        //bends("bend_vals");
        paramList->setParamCache("filepath", &m_filepath);
        paramList->setParamCache("uscale", &m_uscale);
        paramList->setParamCache("vscale", &m_vscale);
        paramList->setParamCache("uvrotation", &m_uvrotation);
        
#define TL_FLOAT_PARAM(name) paramList->setParamCache(#name, &yrn0.name);
#define TL_COLOR_PARAM(name) paramList->setParamCache(#name, &yrn0.name);
		TL_YARN_PARAMETERS
#undef TL_FLOAT_PARAM
#undef TL_COLOR_PARAM
        //paramList->setParamCache("diffuse_color_amount", &yrn0.color_amount);
        
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

	int getBSDFFlags(void) {
		return bsdfFlag_none;
	}

private:
    BRDFPool<BRDFThunderLoomSampler> pool;
    CharString m_filepath;
    float m_uscale, m_vscale, m_uvrotation;
    //DefFloatListParam bends;
    FloatList bends2;

    // Parameters
    PluginBase *baseBRDFParam;
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

    if (type == paramtype_object) {
        //TODO check if it has interface.
		PluginBase* newPlug = param->getObject(i);
		*target = newPlug;
        return 1;
    }
    return 0;
}

int get_bool(VRayPluginParameter * param, int i, VRayContext& rc, double t) {
    if (param->getType(i,0) == VRayParameterType::paramtype_bool && param->getBool(i,t))
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
    // Get filepath
    VRayPluginParameter* filepath_param = this->getParameter("filepath");
	CharString filepath;
	if (filepath_param) {
		filepath = filepath_param->getString();
	}

	const VUtils::VRayFrameData frame_data = vray->getFrameData();
	double time = frame_data.t;

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
        m_tl_wparams->uvrotation = 0.f;
        m_tl_wparams->realworld_uv = 0;
        tl_prepare(m_tl_wparams);
    }
    
    // Make new VRayContext
    VRayThreadData* vr_tdata = vray->getThreadData(0);
    VRayContext rc;
    rc.init(vr_tdata, vray);
	rc.clear();
    
    VRayPluginParameter* uscale = this->getParameter("uscale");
    VRayPluginParameter* vscale = this->getParameter("vscale");
    VRayPluginParameter* uvrotation = this->getParameter("uvrotation");
    m_tl_wparams->uscale = uscale->getFloat(0,time);
    m_tl_wparams->vscale = vscale->getFloat(0,time);
    m_tl_wparams->uvrotation = uvrotation->getFloat(0,time);

    // Yarn type params
#define TL_FLOAT_PARAM(name) \
    VRayPluginParameter* name = this->getParameter(#name);\
	VRayPluginParameter* name##_on = this->getParameter(#name "_on");\
	VRayPluginParameter* name##_on_float = this->getParameter(#name "_on_float");
#define TL_COLOR_PARAM(name) \
    VRayPluginParameter* name = this->getParameter(#name);\
	VRayPluginParameter* name##_on = this->getParameter(#name "_on");\
	VRayPluginParameter* name##_on_float = this->getParameter(#name "_on_float");
		TL_YARN_PARAMETERS
#undef TL_FLOAT_PARAM
#undef TL_COLOR_PARAM
    
    /*
	VRayPluginParameter* diffuse_color_amount = this->getParameter("diffuse_color_amount");
    VRayPluginParameter* diffuse_color_amount_on = this->getParameter("diffuse_color_amount_on");
    VRayPluginParameter* diffuse_color_amount_on_float = this->getParameter("diffuse_color_amount_on_float");
	*/


    // Loop through yarn types and set parameters from list
    for (unsigned int i=0; i < m_tl_wparams->num_yarn_types; i++) {
        //get parameter from config string
        tlYarnType* yarn_type = &m_tl_wparams->yarn_types[i];

#define TL_FLOAT_PARAM(name) \
        if (is_param_valid(name, i)) {\
            if (!set_texparam(name, &yarn_type->name##_texmap, i))\
                yarn_type->name = name->getFloat(i,time);\
        }\
        if (is_param_valid(name##_on, i)) \
            yarn_type->name##_enabled = get_bool(name##_on, i, rc, time); \
        else if (is_param_valid(name##_on_float, i)) \
            yarn_type->name##_enabled = get_bool(name##_on_float, i, rc, time);
#define TL_COLOR_PARAM(name)  \
        if (is_param_valid(name, i)) { \
			if (!set_texparam(name, &yarn_type->name##_texmap, i)) { \
				Color tmp_col = name->getColor(i,time); \
				tlColor tl_col; \
				tl_col.r = tmp_col.r; tl_col.g = tmp_col.g; tl_col.b = tmp_col.b; \
                yarn_type->name = tl_col; \
            } \
        } \
        if (is_param_valid(name##_on, i)) \
            yarn_type->name##_enabled = get_bool(name##_on, i, rc, time); \
        else if (is_param_valid(name##_on_float, i)) \
            yarn_type->name##_enabled = get_bool(name##_on_float, i, rc, time); 
		TL_YARN_PARAMETERS
#undef TL_FLOAT_PARAM
#undef TL_COLOR_PARAM

#undef TL_VRAY_FLOAT_PARAMS
#undef TL_VRAY_SET_PARAM


	/*
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
	*/

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

//TODO(Vidar):Is this used anywhere??
struct tlShadeData: VR::VRayShadeData {
    tlShadeData(float u, float v) {
        this->u = u;
        this->v = v;
    }

    VR::Vector getUVWCoords(const VR::VRayContext &rc, int channel) {
        return VR::Vector(this->u, this->v, 0.f);
    }
    
    void getUVWderivs(const VR::VRayContext &rc, int channel, VR::Vector derivs[2]) {
        derivs[0].makeZero();
        derivs[1].makeZero();
    }
    float u, v;
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


#define BRDFTest_PluginID PluginID(LARGE_CONST(2017041301))
// #ifdef __VRAY40__
// SIMPLE_PLUGIN_LIBRARY(BRDFTest_PluginID, "BRDFTest", "BRDFTestLibText", "a simple test plugin/lib", BRDFTest, BRDFTest_Params, EXT_BSDF);
// #else
SIMPLE_PLUGIN_LIBRARY(BRDFTest_PluginID, "BRDFThunderLoom" ,"BRDFThunderLoom", "BRDFTestLibText", BRDFThunderLoom, BRDFThunderLoomParams, EXT_BSDF );
// #endif

//PLUGIN_DESC(BRDFTest_PluginID, EXT_BSDF, "BRDFTest", BRDFTest, BRDFTest_Params);
