#ifndef __MTLSKEL__H
#define __MTLSKEL__H

#include "max.h"
#include <bmmlib.h>
#include "iparamm2.h"
#include "resource.h"

#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 6000
#include "IMtlRender_Compatibility.h"
#endif
#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 13900
#include <IMaterialBrowserEntryInfo.h>
#endif

#include "vraybase.h"
#include "vrayplugins.h"
#include "brdfs.h"
#include "vraygeom.h"
#include "brdfpool.h"
#include "VrayThunderLoomBRDF.h"
#include "pb2template_generator.h"

#include "thunderloom.h"

//NOTE(Peter):These are parameters for each yarn type that can be varied  
//for each uv position. These paramters are good to vary using texture maps
#define YARN_TYPE_TEXMAP_PARAMETERS\
	YARN_TYPE_TEXMAP(color)\
	YARN_TYPE_TEXMAP(specular_color)\
	YARN_TYPE_TEXMAP(yarnsize)
enum {
#define YARN_TYPE_TEXMAP(param) yrn_texmaps_##param,
	YARN_TYPE_TEXMAP_PARAMETERS
#undef YARN_TYPE_PARAM
	NUMBER_OF_YRN_TEXMAPS
};


// IMPORTANT:
// The ClassID must be changed whenever a new project
// is created using this skeleton
#define	MTL_CLASSID	Class_ID(0x1456441a, 0x58760c4)
#define STR_CLASSNAME _T("ThunderLoom")
#define STR_LIBDESC _T("ThunderLoom woven cloth material")
#define STR_DLGTITLE _T("ThunderLoom Parameters")

extern ClassDesc* GetSkeletonMtlDesc();

// Paramblock2 name
enum { mtl_params, }; 

enum {
	mtl_diffuse,
	mtl_realworld,
    mtl_uscale,
    mtl_vscale,
    mtl_uvrotation,
	mtl_yarn_type,
    mtl_intensity_fineness,
	//mtl_texmap_diffuse,
	//mtl_texmap_specular,
    texmaps, //stores texmaps for each yarntype in a TEXMAPS_TAB pblock
    mtl_dummy,
};

static int texmapBtnIDCs[NUMBER_OF_YRN_TEXMAPS] = 
{
#define YARN_TYPE_TEXMAP(name) IDC_YRN_TEX_##name##_BUTTON,
	YARN_TYPE_TEXMAP_PARAMETERS
#undef YARN_TYPE_TEXMAP
};

/*===========================================================================*\
 |	The actual BRDF
\*===========================================================================*/

//MyBaseBSDF defined in dynamic part is subclass of BSDFSampler
//From vray docs: This class is used to describe both sides
//of an object's surface. 
//An instance of this class is passed to VRayContext::evalLight() 
//to evaluate the contribution of lights for the surface. 
//To completely describe the optical properties of a surface, 
//two BRDFs (one for each side of the surface) are coupled
//together to form a BSDF (bi-directional scattering distribution function). 
class MyBlinnBSDF: public VR::MyBaseBSDF {
public:
};

/*===========================================================================*\
 |	SkeletonMaterial class defn
\*===========================================================================*/

class ThunderLoomMtl : public Mtl, public VR::VRenderMtl {
	VR::BRDFPool<MyBlinnBSDF> bsdfPool;
	VR::LayeredBSDFRenderChannels renderChannels;
	VR::Color getBlend(ShadeContext &sc, int i);
public:
	// various variables
	HWND m_hwMtlEdit;
	IMtlParams *m_imp;

	Interval ivalid;
    tlWeaveParameters *m_weave_parameters;
    IMtlParams *m_i_mtl_params;
    bool m_yarn_type_rollup_open[TL_MAX_YARN_TYPES];

	// Parameter and UI management
	IParamBlock2 *pblock; 	//ref 0
	ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp);
	void Update(TimeValue t, Interval& valid);
	Interval Validity(TimeValue t);
	void Reset();

	ThunderLoomMtl(BOOL loading);
	Class_ID ClassID() { return MTL_CLASSID; }
	SClass_ID SuperClassID() { return MATERIAL_CLASS_ID; }
	void GetClassName(TSTR& s) { s=STR_CLASSNAME; }
	void DeleteThis() { delete this; }
	
	void NotifyChanged();

	// From MtlBase and Mtl
	virtual void SetAmbient(Color c, TimeValue t);		
	virtual void SetDiffuse(Color c, TimeValue t);		
	virtual void SetSpecular(Color c, TimeValue t);
	virtual void SetShininess(float v, TimeValue t);
	virtual Color GetAmbient(int mtlNum=0, BOOL backFace=FALSE);
	virtual Color GetDiffuse(int mtlNum=0, BOOL backFace=FALSE);
	virtual Color GetSpecular(int mtlNum=0, BOOL backFace=FALSE);
	virtual float GetXParency(int mtlNum=0, BOOL backFace=FALSE);
	virtual float GetShininess(int mtlNum=0, BOOL backFace=FALSE);		
	virtual float GetShinStr(int mtlNum=0, BOOL backFace=FALSE);
	virtual float WireSize(int mtlNum=0, BOOL backFace=FALSE);

	//This method is called on a material coming into an existing set of ParamDlgs,
	//once for each secondary ParamDlg and it should set the appropriate 'thing' into
	//the given dlg (the 'thing' being, for example, a Texout* or UVGen*). 
	BOOL SetDlgThing(ParamDlg* dlg);

	// Loading/Saving
	RefTargetHandle Clone(RemapDir& remap);
	IOResult Save(ISave *isave); 
	IOResult Load(ILoad *iload); 

	// SubMaterial access methods
	// We have no sub materials.
	int NumSubMtls() { return 0; }
	Mtl* GetSubMtl(int i) { return NULL; }
	void SetSubMtl(int i, Mtl *m) {}
	TSTR GetSubMtlSlotName(int i) { return _T(""); }
	TSTR GetSubMtlTVName(int i) { return _T(""); }

	// SubTexmap access methods
	int NumSubTexmaps();
	Texmap* GetSubTexmap(int i);
	void SetSubTexmap(int i, Texmap *m);
	TSTR GetSubTexmapSlotName(int i);
	TSTR GetSubTexmapTVName(int i);

	//SubAnims
	// Allows us to update dependent objects
	// which may be animatable. 
	// Such as submtls or SubTexmapsmaterials or texmaps
	int NumSubs() { return 1; } 
	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);
	int SubNumToRefNum(int subNum) { return subNum; }

	// Number of references
	//TODO(Peter): What is this?
 	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int i);
	void SetReference(int i, RefTargetHandle rtarg);
	RefResult NotifyRefChanged(NOTIFY_REF_CHANGED_ARGS);

	// Direct Paramblock2 access
	int	NumParamBlocks() { return 1; }
	IParamBlock2* GetParamBlock(int i) { return pblock; }
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } 

	//From Mtl, uses integers to identify the interface IDs. 
	//Used to determine if an animatable can be used as the type a certain interface is representing
	void* GetInterface(ULONG id) {
		if (id==I_VRAYMTL) return static_cast<VR::VRenderMtl*>(this);
		return Mtl::GetInterface(id);
	}

	// From VRenderMtl
	void renderBegin(TimeValue t, VR::VRayRenderer *vray);
	void renderEnd(VR::VRayRenderer *vray);
	VR::BSDFSampler* newBSDF(const VR::VRayContext &rc, VR::VRenderMtlFlags flags);
	void deleteBSDF(const VR::VRayContext &rc, VR::BSDFSampler *bsdf);
	void addRenderChannel(int index);
	VR::VRayVolume* getVolume(const VR::VRayContext &rc);

	// Shade and displacement calculation
	virtual void Shade(ShadeContext& sc);
	virtual float EvalDisplacement(ShadeContext& sc); 
	virtual Interval DisplacementValidity(TimeValue t); 
};
#endif