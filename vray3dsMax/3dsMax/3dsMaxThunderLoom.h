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

#include "woven_cloth.h"

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

// Paramblock2 parameter list
/*
enum {
	mtl_testparam,
};*/


enum {
	mtl_color,
	mtl_diffuse,
    mtl_umax,
	mtl_realworld,
    mtl_uscale,
    mtl_vscale,
    mtl_psi,
    mtl_delta_x,
    mtl_alpha,
    mtl_beta,
    mtl_wiffile,
    mtl_specular,
    mtl_intensity_fineness,
	mtl_yarnvar_amplitude,
    mtl_yarnvar_xscale,
    mtl_yarnvar_yscale,
    mtl_yarnvar_persistance,
    mtl_yarnvar_octaves,
    mtl_texture,
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
	Interval ivalid;
    wcWeaveParameters m_weave_parameters;
	ParamBlockDesc2  *m_param_blocks;

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
	// We have no sub Texmaps
	int NumSubTexmaps() { return 0; }
	Texmap* GetSubTexmap(int i) { return NULL; }
	void SetSubTexmap(int i, Texmap *m) {}
	TSTR GetSubTexmapSlotName(int i) { return _T(""); }
	TSTR GetSubTexmapTVName(int i) { return _T(""); }

	// Number of subanims
	//TODO(Peter): What is this?
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
	int	NumParamBlocks() { return 2; }
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