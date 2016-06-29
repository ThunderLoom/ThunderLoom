//3dsMaxThunderLoom.cpp
// This file sets up and registers the 3dsMax plugin.

#include "dynamic.h"

#include "max.h"

#include "vrayinterface.h" //BSDFsampler amongst others..
#include "vrender_unicode.h"

#include "3dsMaxThunderLoom.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include "helper.h"

#ifdef DYNAMIC
EVALSPECULARFUNC EvalSpecularFunc = 0;
EVALDIFFUSEFUNC EvalDiffuseFunc = 0;
#endif

// no param block script access for VRay free
// TODO(Peter): VRay free? Demo?
#ifdef _FREE_
#define _FT(X) _T("")
#define IS_PUBLIC 0
#else
#define _FT(X) _T(X)
#define IS_PUBLIC 1
#endif // _FREE_

/*===========================================================================*\
 |	Class Descriptor
\*===========================================================================*/
//The class descriptor registers plugin with 3dsMax. 

class ThunderLoomClassDesc : public ClassDesc2 
#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 6000
, public IMtlRender_Compatibility_MtlBase 
	//This interface is used to determine whether
	//a material/map flags itself as being compatible
	//with a specific renderer plugin.
	//The interface also provides a way of defining
	//an icon that appears in the material browser
	//- GetCustomMtlBrowserIcon. 
#endif
#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 13900
, public IMaterialBrowserEntryInfo
//This interface allows materials and textures to customize their
//appearance in the Material Browser
//If implemented, the plug-in class should
//return its instance of this interface in response to
//ClassDesc::GetInterface(IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE). 
//The interface allows a plug-in to customize the default
//appearance of its entries, as shown in the Material Browser.
//This includes the display name, the thumbnail, and the location
//(or category) of entries.
#endif
{
	HIMAGELIST imageList;
public:
	virtual int IsPublic() 							{ return IS_PUBLIC; }
	virtual void* Create(BOOL loading = FALSE)		{ return new ThunderLoomMtl(loading); }
	virtual const TCHAR *	ClassName() 			{ return STR_CLASSNAME; }
	virtual SClass_ID SuperClassID() 				{ return MATERIAL_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return MTL_CLASSID; }
	virtual const TCHAR* Category() 				{ return NULL; }
	virtual const TCHAR* InternalName() 			{ return _T("thunderLoomMtl"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle

	ThunderLoomClassDesc(void) {
		imageList=NULL;
#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 6000
		IMtlRender_Compatibility_MtlBase::Init(*this);
#endif
	}
	~ThunderLoomClassDesc(void) {
		if (imageList) ImageList_Destroy(imageList);
		imageList=NULL;
	}

#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 6000 //NOTE(Peter): have a minimum version requirement? minimum VERSION_3DSMAX = 1490`0?
	// From IMtlRender_Compatibility_MtlBase
	bool IsCompatibleWithRenderer(ClassDesc& rendererClassDesc) {
		if (rendererClassDesc.ClassID()!=VRENDER_CLASS_ID) return false;
		return true;
	}
	bool GetCustomMtlBrowserIcon(HIMAGELIST& hImageList, int& inactiveIndex, int& activeIndex, int& disabledIndex) {
		if (!imageList) {
			HBITMAP bmp=LoadBitmap(hInstance, MAKEINTRESOURCE(bm_MTL));
			HBITMAP mask=LoadBitmap(hInstance, MAKEINTRESOURCE(bm_MTLMASK));

			imageList=ImageList_Create(11, 11, ILC_COLOR24 | ILC_MASK, 5, 0);
			int index=ImageList_Add(imageList, bmp, mask);
			if (index==-1) return false;
		}

		if (!imageList) return false;

		hImageList=imageList;
		inactiveIndex=0;
		activeIndex=1;
		disabledIndex=2;
		return true;
	} //TODO(Peter): Is this needed?
#endif

#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 13900
	FPInterface* GetInterface(Interface_ID id) {
		if (IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE==id) {
			return static_cast<IMaterialBrowserEntryInfo*>(this);
		}
		return ClassDesc2::GetInterface(id);
	}
	// From IMaterialBrowserEntryInfo
	const MCHAR* GetEntryName() const { return NULL; }
	const MCHAR* GetEntryCategory() const {
#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 14900
		HINSTANCE hInst=GetModuleHandle(_T("sme.gup"));
		if (hInst) {
			static MSTR category(MaxSDK::GetResourceStringAsMSTR(hInst, IDS_3DSMAX_SME_MATERIALS_CATLABEL).Append(_T("\\V-Ray"))); //Extract a resource from the calling module's string table.
			return category.data();
		}
#endif
		return _T("Materials\\V-Ray");
	}
	Bitmap* GetEntryThumbnail() const { return NULL; }
#endif
};

//Make instance of Plugin ClassDescriptor
static ThunderLoomClassDesc thunderLoomDesc;
ClassDesc* GetSkeletonMtlDesc() {return &thunderLoomDesc;}

/*===========================================================================*\
 |	Basic implimentation of a dialog handler
\*===========================================================================*/

/*===========================================================================*\
 |	Paramblock2 Descriptor
\*===========================================================================*/

// Used for vray auto ui
//static int numID=100;
//int ctrlID(void) { return numID++; }

static ParamBlockDesc2 thunderLoom_param_blk (mtl_params, _T("Test mtl params"), 0, &thunderLoomDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, 0,
	//rollout
	IDD_BLENDMTL, IDS_PARAMETERS, 0, 0, NULL, 
	//params
    mtl_wiffile, _FT("wifFile"), TYPE_FILENAME, P_ANIMATABLE, 0,
        p_default, _FT(""),
		p_ui, TYPE_FILEOPENBUTTON, IDC_WIFFILE_BUTTON,
	PB_END,
	// pattern an geometry
    mtl_uscale, _FT("uscale"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 1.f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_USCALE_EDIT, IDC_USCALE_SPIN, 0.1f,
	PB_END,
    mtl_vscale, _FT("vscale"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 1.f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_VSCALE_EDIT, IDC_VSCALE_SPIN, 0.1f,
	PB_END,
	mtl_realworld, _FT("realworld"), TYPE_BOOL, 0, 0,
		p_default, FALSE,
		p_ui, TYPE_SINGLECHEKBOX, IDC_REALWORLD_CHECK,
	PB_END,
    mtl_umax, _FT("bend"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 0.5,
		p_range, 0.0f, 1.f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_UMAX_EDIT, IDC_UMAX_SPIN, 0.1f,
	PB_END,
    mtl_psi, _FT("twist"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 0.5,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_PSI_EDIT, IDC_PSI_SPIN, 0.1f,
	PB_END,
	//Lighting params
	mtl_specular, _FT("specular"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default,		1.f,
		p_range,		0.0f, 1.f,
		p_ui,			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SPECULAR_EDIT, IDC_SPECULAR_SPIN, 0.1f,
	p_end,
    mtl_delta_x, _FT("highligtWidth"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 0.5f,
        p_range, 0.0f, 1.f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_DELTAX_EDIT, IDC_DELTAX_SPIN, 0.1f,
	PB_END,
    mtl_alpha, _FT("alpha"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 0.05,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_ALPHA_EDIT, IDC_ALPHA_SPIN, 0.1f,
	PB_END,
    mtl_beta, _FT("beta"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 4.f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_BETA_EDIT, IDC_BETA_SPIN, 0.1f,
	PB_END,
PB_END
);
											  

/*
	distortion_str,	_T("distortionStr"),		 TYPE_FLOAT,	P_ANIMATABLE,	IDS_DISTORTION_STRENGTH,
		p_default,		0.1f,
		p_range,		0.0f, 1000.0f,
		p_ui, 			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_DISTORTION_EDIT, IDC_DISTORTION_SPIN, 0.001f, 
		p_end,
*/

/*
static ParamBlockDesc2 smtl_param_blk ( mtl_params, _T("SkeletonMaterial parameters"),  0, &SkelMtlCD, P_AUTO_CONSTRUCT + P_AUTO_UI, 0, 
	//rollout
	IDD_SKELETON_MATERIAL, IDS_PARAMETERS, 0, 0, NULL, 
	// params
	mtl_diffuse, _FT("diffuse"), TYPE_RGBA, P_ANIMATABLE, 0,
		p_default, Color(0.5f, 0.5f, 0.5f),
		p_ui, TYPE_COLORSWATCH, ctrlID(),
	PB_END,
    mtl_specular, _FT("specular"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 1.f,
		p_range, 0.0f, 1.f,
		
	PB_END,
    mtl_umax, _FT("bend"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 0.5,
		p_range, 0.0f, 1.f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, ctrlID(), ctrlID(), 0.1f,
	PB_END,
	mtl_realworld, _FT("realworld"), TYPE_BOOL, 0, 0,
		p_default, FALSE,
		p_ui, TYPE_SINGLECHEKBOX, ctrlID(),
	PB_END,
    mtl_uscale, _FT("uscale"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 1.f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, ctrlID(), ctrlID(), 0.1f,
	PB_END,
    mtl_vscale, _FT("vscale"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 1.f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, ctrlID(), ctrlID(), 0.1f,
	PB_END,
    mtl_psi, _FT("twist"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 0.5,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, ctrlID(), ctrlID(), 0.1f,
	PB_END,
    mtl_delta_x, _FT("highligtWidth"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 0.5f,
        p_range, 0.0f, 1.f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, ctrlID(), ctrlID(), 0.1f,
	PB_END,
    mtl_alpha, _FT("alpha"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 0.05,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, ctrlID(), ctrlID(), 0.1f,
	PB_END,
    mtl_beta, _FT("beta"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 4.f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, ctrlID(), ctrlID(), 0.1f,
	PB_END,
    mtl_intensity_fineness, _FT("intensity_fineness"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 1.f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, ctrlID(), ctrlID(), 0.1f,
	PB_END,
	mtl_yarnvar_amplitude, _FT("yarnvar_amplitude"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 0.f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, ctrlID(), ctrlID(), 0.1f,
	PB_END,
	mtl_yarnvar_xscale, _FT("yarnvar_xscale"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 1.f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, ctrlID(), ctrlID(), 0.1f,
	PB_END,
	mtl_yarnvar_yscale, _FT("yarnvar_yscale"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 1.f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, ctrlID(), ctrlID(), 0.1f,
	PB_END,
	mtl_yarnvar_persistance, _FT("yarnvar_persistance"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 1.f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, ctrlID(), ctrlID(), 0.1f,
	PB_END,
	mtl_yarnvar_octaves, _FT("yarnvar_octaves"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 1.f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, ctrlID(), ctrlID(), 1.f,
	PB_END,
    mtl_wiffile, _FT("wifFile"), TYPE_FILENAME, P_ANIMATABLE, 0,
        p_default, _FT(""),
		p_ui, TYPE_FILEOPENBUTTON, ctrlID(),
	PB_END,
    mtl_texture, _FT("texture"), TYPE_TEXMAP, 0, 0,
		p_ui, TYPE_TEXMAPBUTTON, ctrlID(),
	PB_END,
PB_END
);
/*

/*===========================================================================*\
 |	UI stuff
\*===========================================================================*/

//callbacks for messages.
//TODO(Peter): better reason for keeping!
class ThunderLoomMtlDlgProc : public ParamMap2UserDlgProc {
public:
	IParamMap *pmap;
	ThunderLoomMtl *sm;

	ThunderLoomMtlDlgProc(void) { sm = NULL; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		int id = LOWORD(wParam);
		switch (msg) 
		{
			case WM_INITDIALOG:{ 
				DebugPrint(L"INIT DIALOG\n");
				//intercept message that dialog is being init
				//and use this signal to reload the dynamic dll
#ifdef DYNAMIC
                unload_dlls();
                DebugPrint(L"Unloaded dll\n"); //Dlls are loaded in renderBegin
#endif
				break;
            }
			case WM_DESTROY:
				break;
			case WM_COMMAND: 
                break;
		}
		return FALSE;
	}
	void DeleteThis() {}
	void SetThing(ReferenceTarget *m) { sm = (ThunderLoomMtl*)m; }
};

static ThunderLoomMtlDlgProc dlgProc;

//TODO(Peter): make own ParamDlg?
/*
class SkelMtlParamDlg: public ParamDlg {
public:
	SkeletonMaterial *mtl;
	IMtlParams *ip;
	IParamMap2 *pmap;
	
	SkelMtlParamDlg(SkeletonMaterial *m, HWND hWnd, IMtlParams *i) {
		mtl=m;
		ip=i;

		
		//DLGTEMPLATE* tmp=templateGenerator.GenerateTemplate(mtl->pblock, STR_DLGTITLE, 217);
		//pmap=CreateMParamMap2(mtl->pblock, ip, hInstance, hWnd, NULL, NULL, tmp, STR_DLGTITLE, 0, &dlgProc);
		//templateGenerator.ReleaseDlgTemplate(tmp);
	}

	void DeleteThis(void) {
		if (pmap) DestroyMParamMap2(pmap);
		pmap=NULL;
		delete this;
	}
	
	Class_ID ClassID(void) { return MTL_CLASSID; }
	void SetThing(ReferenceTarget *m) { mtl=(SkeletonMaterial*) m; pmap->SetParamBlock(mtl->pblock); }
	ReferenceTarget* GetThing(void) { return mtl; }
	void SetTime(TimeValue t) {}
	void ReloadDialog(void) {}
	void ActivateDlg(BOOL onOff) {}
};
*/
/*===========================================================================*\
 |	Constructor and Reset systems
 |  Ask the ClassDesc2 to make the AUTO_CONSTRUCT paramblocks and wire them in
\*===========================================================================*/

void ThunderLoomMtl::Reset() {
	ivalid.SetEmpty();
	thunderLoomDesc.Reset(this);
}

ThunderLoomMtl::ThunderLoomMtl(BOOL loading) {
	pblock=NULL;
	ivalid.SetEmpty();
	thunderLoomDesc.MakeAutoParamBlocks(this);	// make and intialize paramblock2
	Reset();
}

ParamDlg* ThunderLoomMtl::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp) {
	//return new SkelMtlParamDlg(this, hwMtlEdit, imp);
	
	IAutoMParamDlg* masterDlg = thunderLoomDesc.CreateParamDlgs(hwMtlEdit, imp, this);
	//thunderLoom_param_blk.SetUserDlgProc(new SkelMtlDlgProc(this));
	thunderLoom_param_blk.SetUserDlgProc(new ThunderLoomMtlDlgProc());
	return masterDlg;
	
}

BOOL ThunderLoomMtl::SetDlgThing(ParamDlg* dlg) {
	return FALSE;
}

Interval ThunderLoomMtl::Validity(TimeValue t) {
	Interval temp=FOREVER;
	Update(t, temp);
	return ivalid;
}

/*===========================================================================*\
 |	Subanim & References support
\*===========================================================================*/

RefTargetHandle ThunderLoomMtl::GetReference(int i) {
	if (i==0) return pblock;
	return NULL;
}

void ThunderLoomMtl::SetReference(int i, RefTargetHandle rtarg) {
	if (i==0) pblock=(IParamBlock2*) rtarg;
}

TSTR ThunderLoomMtl::SubAnimName(int i) {
	if (i==0) return _T("Parameters");
	return _T("");
}

Animatable* ThunderLoomMtl::SubAnim(int i) {
	if (i==0) return pblock;
	return NULL;
}

RefResult ThunderLoomMtl::NotifyRefChanged(NOTIFY_REF_CHANGED_ARGS) {
	switch (message) {
		case REFMSG_CHANGE:
			ivalid.SetEmpty();
			if (hTarget==pblock) {
				ParamID changing_param = pblock->LastNotifyParamID();
				//smtl_param_blk.InvalidateUI(changing_param);
				thunderLoom_param_blk.InvalidateUI(changing_param);
			}
			break;
	}
	return(REF_SUCCEED);
}

/*===========================================================================*\
 |	Standard IO
\*===========================================================================*/

#define MTL_HDR_CHUNK 0x4000

IOResult ThunderLoomMtl::Save(ISave *isave) { 
	IOResult res;
	isave->BeginChunk(MTL_HDR_CHUNK);
	res = MtlBase::Save(isave);
	if (res!=IO_OK) return res;
	isave->EndChunk();
	return IO_OK;
}	

IOResult ThunderLoomMtl::Load(ILoad *iload) { 
	IOResult res;
	int id;
	while (IO_OK==(res=iload->OpenChunk())) {
		switch(id = iload->CurChunkID())  {
			case MTL_HDR_CHUNK:
				res = MtlBase::Load(iload);
				break;
		}
		iload->CloseChunk();
		if (res!=IO_OK) return res;
	}
	return IO_OK;
}

/*===========================================================================*\
 |	Updating and cloning
\*===========================================================================*/

RefTargetHandle ThunderLoomMtl::Clone(RemapDir &remap) {
	ThunderLoomMtl *mnew = new ThunderLoomMtl(FALSE);
	*((MtlBase*)mnew) = *((MtlBase*)this); 
	BaseClone(this, mnew, remap);
	mnew->ReplaceReference(0, remap.CloneRef(pblock));
	mnew->ivalid.SetEmpty();	
	return (RefTargetHandle) mnew;
}

void ThunderLoomMtl::NotifyChanged() {
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void ThunderLoomMtl::Update(TimeValue t, Interval& valid) {
	//if (!ivalid.InInterval(t)) {
	//	ivalid.SetInfinite();
	// ...
	//}

	//valid &= ivalid;
}

/*===========================================================================*\
 |	Determine the characteristics of the material
\*===========================================================================*/

void ThunderLoomMtl::SetAmbient(Color c, TimeValue t) {}		
void ThunderLoomMtl::SetDiffuse(Color c, TimeValue t) {}		
void ThunderLoomMtl::SetSpecular(Color c, TimeValue t) {}
void ThunderLoomMtl::SetShininess(float v, TimeValue t) {}
				
Color ThunderLoomMtl::GetAmbient(int mtlNum, BOOL backFace) {
	return Color(0,0,0);
}

Color ThunderLoomMtl::GetDiffuse(int mtlNum, BOOL backFace) {
	return Color(0.8f, 0.8f, 0.8f);
}

Color ThunderLoomMtl::GetSpecular(int mtlNum, BOOL backFace) {
	return Color(0,0,0);
}

float ThunderLoomMtl::GetXParency(int mtlNum, BOOL backFace) {
	return 0.0f;
}

float ThunderLoomMtl::GetShininess(int mtlNum, BOOL backFace) {
	return 0.0f;
}

float ThunderLoomMtl::GetShinStr(int mtlNum, BOOL backFace) {
	return 0.0f;
}

float ThunderLoomMtl::WireSize(int mtlNum, BOOL backFace) {
	return 0.0f;
}

// Render intialization and deinitialization

void ThunderLoomMtl::renderBegin(TimeValue t, VR::VRayRenderer *vray) {
	ivalid.SetInfinite();

#ifdef DYNAMIC
	unload_dlls();
	EvalDiffuseFunc  = (EVALDIFFUSEFUNC) get_dynamic_func("eval_diffuse" );
	EvalSpecularFunc = (EVALSPECULARFUNC)get_dynamic_func("eval_specular");
#endif

	pblock->GetValue(mtl_uscale,t, m_weave_parameters.uscale,ivalid);
    pblock->GetValue(mtl_vscale,t, m_weave_parameters.vscale,ivalid);
	int realworld; 
	pblock->GetValue(mtl_realworld,t, realworld ,ivalid);
	m_weave_parameters.realworld_uv = realworld;
	pblock->GetValue(mtl_umax,t, m_weave_parameters.umax,ivalid);
	pblock->GetValue(mtl_psi,t, m_weave_parameters.psi,ivalid);
	pblock->GetValue(mtl_specular, t, m_weave_parameters.specular_strength, ivalid);
	pblock->GetValue(mtl_delta_x,t, m_weave_parameters.delta_x,ivalid);
    pblock->GetValue(mtl_alpha,t, m_weave_parameters.alpha,ivalid);
    pblock->GetValue(mtl_beta,t, m_weave_parameters.beta,ivalid);

	DBOUT( "Updating params, uscale: " << uscale );
	DBOUT( "Updating params, vscale: " << vscale );
	DBOUT( "Updating params, realworld: " << realworld );
	DBOUT( "Updating params, umax: " << umax );
	DBOUT( "Updating params, psi: " << psi );
	DBOUT( "Updating params, specular: " << specular );
	DBOUT( "Updating params, delta_x: " << delta_x );
	DBOUT( "Updating params, alpha: " << alpha );
	DBOUT( "Updating params, beta: " << beta );

	//TODO(Peter): What to do about diffuse?
	//set default to white for now.
	//diffuse.White();

	//default values for the time being.
	//these paramters will be removed later, is the plan
	m_weave_parameters.specular_normalization = 1.f;
	m_weave_parameters.intensity_fineness = 1.f;
	m_weave_parameters.yarnvar_amplitude = 0.f;
	m_weave_parameters.yarnvar_xscale = 1.f;
	m_weave_parameters.yarnvar_yscale = 1.f;
	m_weave_parameters.yarnvar_persistance = 1.f;
	m_weave_parameters.yarnvar_octaves = 1;

	//Load wif file.
	//TODO(Peter): Move loading from renderBegin to Update.
	//Will need to load file, and update ui based on content.
    MSTR filename = pblock->GetStr(mtl_wiffile,t);
    wcWeavePatternFromFile_wchar(&m_weave_parameters,filename);

	DBOUT( "begin render params, specular: " << specular );
	DBOUT( "begin render params, filename: " << filename );

	const VR::VRaySequenceData &sdata=vray->getSequenceData();
	bsdfPool.init(sdata.maxRenderThreads);
}

void ThunderLoomMtl::renderEnd(VR::VRayRenderer *vray) {
    //Free pattern
	wcFreeWeavePattern(&m_weave_parameters);

	bsdfPool.freeMem();
	renderChannels.freeMem();
}

/*===========================================================================*\
 |	Actual shading takes place
\*===========================================================================*/

void ThunderLoomMtl::Shade(ShadeContext &sc) {
	if (sc.ClassID()==VRAYCONTEXT_CLASS_ID)
		shade(static_cast<VR::VRayInterface&>(sc), gbufID);
	else {
		if (gbufID) sc.SetGBufferID(gbufID);
		sc.out.c.Black(); sc.out.t.Black();
	}
}

float ThunderLoomMtl::EvalDisplacement(ShadeContext& sc)
{
	return 0.0f;
}

Interval ThunderLoomMtl::DisplacementValidity(TimeValue t)
{
	Interval iv;
	iv.SetInfinite();
	return iv;
}
	
