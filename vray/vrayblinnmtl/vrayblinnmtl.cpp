#include "dynamic.h"

#include "max.h"

#include "vrayinterface.h"
#include "vrender_unicode.h"

#include "vrayblinnmtl.h"

#define _USE_MATH_DEFINES
#include <math.h>

EVALFUNC EvalFunc = 0;


// no param block script access for VRay free
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

class SkeletonMaterialClassDesc: public ClassDesc2
#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 6000
, public IMtlRender_Compatibility_MtlBase 
#endif
#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 13900
, public IMaterialBrowserEntryInfo
#endif
{
	HIMAGELIST imageList;
public:
	int IsPublic() { return IS_PUBLIC; }
	void* Create(BOOL loading) { return new SkeletonMaterial(loading); }
	const TCHAR *	ClassName() { return STR_CLASSNAME; }
	SClass_ID SuperClassID() { return MATERIAL_CLASS_ID; }
	Class_ID ClassID() { return MTL_CLASSID; }
	const TCHAR* Category() { return _T("");  }

	// Hardwired name, used by MAX Script as unique identifier
	const TCHAR* InternalName() { return STR_CLASSNAME; }
	HINSTANCE HInstance() { return hInstance; }

	SkeletonMaterialClassDesc(void) {
		imageList=NULL;
#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 6000
		IMtlRender_Compatibility_MtlBase::Init(*this);
#endif
	}

	~SkeletonMaterialClassDesc(void) {
		if (imageList) ImageList_Destroy(imageList);
		imageList=NULL;
	}

#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 6000
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
	}
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
			static MSTR category(MaxSDK::GetResourceStringAsMSTR(hInst, IDS_3DSMAX_SME_MATERIALS_CATLABEL).Append(_T("\\V-Ray")));
			return category.data();
		}
#endif
		return _T("Materials\\V-Ray");
	}
	Bitmap* GetEntryThumbnail() const { return NULL; }
#endif
};

static SkeletonMaterialClassDesc SkelMtlCD;
ClassDesc* GetSkeletonMtlDesc() {return &SkelMtlCD;}

/*===========================================================================*\
 |	Basic implimentation of a dialog handler
\*===========================================================================*/

/*===========================================================================*\
 |	Paramblock2 Descriptor
\*===========================================================================*/

static int numID=100;
int ctrlID(void) { return numID++; }

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
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, ctrlID(), ctrlID(), 0.1f,
	PB_END,
    mtl_umax, _FT("bend"), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_default, 0.5,
		p_range, 0.0f, 1.f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, ctrlID(), ctrlID(), 0.1f,
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

/*===========================================================================*\
 |	UI stuff
\*===========================================================================*/

class SkelMtlDlgProc : public ParamMap2UserDlgProc {
public:
	IParamMap *pmap;
	SkeletonMaterial *sm;

	SkelMtlDlgProc(void) { sm = NULL; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		int id = LOWORD(wParam);
		switch (msg) 
		{
			case WM_INITDIALOG:{
#ifdef DYNAMIC
                unload_dlls();

                DebugPrint(L"Unloaded dll\n");
    
                EvalFunc = (EVALFUNC)get_dynamic_func("dynamic_eval");
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
	void SetThing(ReferenceTarget *m) { sm = (SkeletonMaterial*)m; }
};

static SkelMtlDlgProc dlgProc;
static Pb2TemplateGenerator templateGenerator;

class SkelMtlParamDlg: public ParamDlg {
public:
	SkeletonMaterial *mtl;
	IMtlParams *ip;
	IParamMap2 *pmap;
	
	SkelMtlParamDlg(SkeletonMaterial *m, HWND hWnd, IMtlParams *i) {
		mtl=m;
		ip=i;

		
		DLGTEMPLATE* tmp=templateGenerator.GenerateTemplate(mtl->pblock, STR_DLGTITLE, 217);
		pmap=CreateMParamMap2(mtl->pblock, ip, hInstance, hWnd, NULL, NULL, tmp, STR_DLGTITLE, 0, &dlgProc);
		templateGenerator.ReleaseDlgTemplate(tmp);
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

/*===========================================================================*\
 |	Constructor and Reset systems
 |  Ask the ClassDesc2 to make the AUTO_CONSTRUCT paramblocks and wire them in
\*===========================================================================*/

void SkeletonMaterial::Reset() {
	ivalid.SetEmpty();
	SkelMtlCD.Reset(this);
}

SkeletonMaterial::SkeletonMaterial(BOOL loading) {
	pblock=NULL;
	ivalid.SetEmpty();
	SkelMtlCD.MakeAutoParamBlocks(this);	// make and intialize paramblock2
}

ParamDlg* SkeletonMaterial::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp) {
	return new SkelMtlParamDlg(this, hwMtlEdit, imp);
	/*
	IAutoMParamDlg* masterDlg = SkelMtlCD.CreateParamDlgs(hwMtlEdit, imp, this);
	smtl_param_blk.SetUserDlgProc(new SkelMtlDlgProc(this));
	return masterDlg;
	*/
}

BOOL SkeletonMaterial::SetDlgThing(ParamDlg* dlg) {
	return FALSE;
}

Interval SkeletonMaterial::Validity(TimeValue t) {
	Interval temp=FOREVER;
	Update(t, temp);
	return ivalid;
}

/*===========================================================================*\
 |	Subanim & References support
\*===========================================================================*/

RefTargetHandle SkeletonMaterial::GetReference(int i) {
	if (i==0) return pblock;
	return NULL;
}

void SkeletonMaterial::SetReference(int i, RefTargetHandle rtarg) {
	if (i==0) pblock=(IParamBlock2*) rtarg;
}

TSTR SkeletonMaterial::SubAnimName(int i) {
	if (i==0) return _T("Parameters");
	return _T("");
}

Animatable* SkeletonMaterial::SubAnim(int i) {
	if (i==0) return pblock;
	return NULL;
}

RefResult SkeletonMaterial::NotifyRefChanged(NOTIFY_REF_CHANGED_ARGS) {
	switch (message) {
		case REFMSG_CHANGE:
			ivalid.SetEmpty();
			if (hTarget==pblock) {
				ParamID changing_param = pblock->LastNotifyParamID();
				smtl_param_blk.InvalidateUI(changing_param);
			}
			break;
	}
	return(REF_SUCCEED);
}

/*===========================================================================*\
 |	Standard IO
\*===========================================================================*/

#define MTL_HDR_CHUNK 0x4000

IOResult SkeletonMaterial::Save(ISave *isave) { 
	IOResult res;
	isave->BeginChunk(MTL_HDR_CHUNK);
	res = MtlBase::Save(isave);
	if (res!=IO_OK) return res;
	isave->EndChunk();
	return IO_OK;
}	

IOResult SkeletonMaterial::Load(ILoad *iload) { 
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

RefTargetHandle SkeletonMaterial::Clone(RemapDir &remap) {
	SkeletonMaterial *mnew = new SkeletonMaterial(FALSE);
	*((MtlBase*)mnew) = *((MtlBase*)this); 
	BaseClone(this, mnew, remap);
	mnew->ReplaceReference(0, remap.CloneRef(pblock));
	mnew->ivalid.SetEmpty();	
	return (RefTargetHandle) mnew;
}

void SkeletonMaterial::NotifyChanged() {
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void SkeletonMaterial::Update(TimeValue t, Interval& valid) {
	if (!ivalid.InInterval(t)) {
		ivalid.SetInfinite();
		pblock->GetValue(mtl_diffuse, t, diffuse, ivalid);

        pblock->GetValue(mtl_umax,t, umax,ivalid);
        pblock->GetValue(mtl_uscale,t, uscale,ivalid);
        pblock->GetValue(mtl_vscale,t, vscale,ivalid);
        pblock->GetValue(mtl_psi,t, psi,ivalid);
        pblock->GetValue(mtl_delta_x,t, delta_x,ivalid);
        pblock->GetValue(mtl_alpha,t, alpha,ivalid);
        pblock->GetValue(mtl_beta,t, beta,ivalid);
        pblock->GetValue(mtl_specular,t, specular,ivalid);
        pblock->GetValue(mtl_intensity_fineness,t, intensity_fineness,ivalid);
		pblock->GetValue(mtl_yarnvar_amplitude,t, yarnvar_amplitude,ivalid);
		pblock->GetValue(mtl_yarnvar_xscale,t, yarnvar_xscale,ivalid);
		pblock->GetValue(mtl_yarnvar_yscale,t, yarnvar_yscale,ivalid);
		pblock->GetValue(mtl_yarnvar_persistance,t, yarnvar_persistance,ivalid);
		pblock->GetValue(mtl_yarnvar_octaves,t, yarnvar_octaves,ivalid);
	}

	valid &= ivalid;
}

void SkeletonMaterial::renderBegin(TimeValue t, VR::VRayRenderer *vray) {
    m_weave_parameters.uscale = uscale;
    m_weave_parameters.vscale = vscale;
    m_weave_parameters.umax   = umax;
    m_weave_parameters.psi    = psi;
    m_weave_parameters.alpha  = alpha;
    m_weave_parameters.beta   = beta;
    m_weave_parameters.delta_x = delta_x;
    m_weave_parameters.specular_strength = specular;
    m_weave_parameters.specular_normalization = 1.f;
	m_weave_parameters.intensity_fineness = intensity_fineness;
	m_weave_parameters.yarnvar_amplitude = yarnvar_amplitude;
	m_weave_parameters.yarnvar_xscale = yarnvar_xscale;
	m_weave_parameters.yarnvar_yscale = yarnvar_yscale;
	m_weave_parameters.yarnvar_persistance = yarnvar_persistance;
	m_weave_parameters.yarnvar_octaves = (int)yarnvar_octaves;

    MSTR filename = pblock->GetStr(mtl_wiffile,t);
    wcWeavePatternFromWIF_wchar(&m_weave_parameters,filename);

	const VR::VRaySequenceData &sdata=vray->getSequenceData();
	bsdfPool.init(sdata.maxRenderThreads);
}

void SkeletonMaterial::renderEnd(VR::VRayRenderer *vray) {
    //TODO(Vidar): Free pattern
	bsdfPool.freeMem();
	renderChannels.freeMem();
}
