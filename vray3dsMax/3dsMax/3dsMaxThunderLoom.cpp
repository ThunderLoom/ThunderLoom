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
//This interface is used to determine whether
//a material/map flags itself as being compatible
//with a specific renderer plugin.
//The interface also provides a way of defining
//an icon that appears in the material browser
//- GetCustomMtlBrowserIcon. 
, public IMtlRender_Compatibility_MtlBase 
#endif
#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 13900
//This interface allows materials and textures to customize their
//appearance in the Material Browser
//If implemented, the plug-in class should
//return its instance of this interface in response to
//ClassDesc::GetInterface(IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE). 
//The interface allows a plug-in to customize the default
//appearance of its entries, as shown in the Material Browser.
//This includes the display name, the thumbnail, and the location
//(or category) of entries.
, public IMaterialBrowserEntryInfo
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

#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 6000 //NOTE(Peter): have a minimum version requirement? minimum VERSION_3DSMAX = 14900?
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
	} //TODO(Peter): Is this needed? Do we want custom icon?
#endif

#if GET_MAX_RELEASE(VERSION_3DSMAX) >= 13900
	//Get material to show up in vray section of the material browser.
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
 |	UI stuff
\*===========================================================================*/

static void UpdateYarnTypeParameters(int yarn_type_id, IParamBlock2 *pblock,
	wcWeaveParameters *weave_params, TimeValue t)
{
	YarnType yarn_type = weave_params->pattern->yarn_types[yarn_type_id];
	if (pblock) {
		DBOUT("tex pointer: " << pblock->GetTexmap(texmaps_diffuse,t,yarn_type_id));

		pblock->SetValue(yrn_color, t, Point3(yarn_type.color[0],
			yarn_type.color[1], yarn_type.color[2]));
		
#define YARN_TYPE_PARAM(param) pblock->SetValue(yrn_##param, t,yarn_type.param);
		YARN_TYPE_PARAMETERS

		pblock->SetValue(mtl_uscale, t, weave_params->uscale);
		pblock->SetValue(mtl_vscale, t, weave_params->vscale);

		//texmaps
		Texmap *tex;
		if(tex = pblock->GetTexmap(texmaps_diffuse,t,yarn_type_id)) {
			DBOUT("tex*: " << pblock->GetTexmap(texmaps_diffuse,t,yarn_type_id));
			pblock->SetValue(mtl_texmap_diffuse, t, tex);
		} else {
			pblock->SetValue(mtl_texmap_diffuse, t, NULL);
		}
			
	}
}

//Here there is the option to add custom behaviour to certain messages.
class ThunderLoomMtlDlgProc : public ParamMap2UserDlgProc {
public:
	IParamMap *pmap;
	ThunderLoomMtl *sm;
	HWND m_hWnd;
	wchar_t directory[512];

	void update_yarn_type_combo()
	{
		HWND yarn_type_combo_hwnd = GetDlgItem(m_hWnd,
			IDC_YARNTYPE_COMBO);
		ComboBox_ResetContent(yarn_type_combo_hwnd);
		if (sm && sm->m_weave_parameters.pattern) {
			for (int i = 0; i <
				sm->m_weave_parameters.pattern->num_yarn_types;
				i++)
			{
				wchar_t buffer[128];
				swprintf(buffer, L"Yarn type %d", i);
				ComboBox_AddString(yarn_type_combo_hwnd, buffer);
			}
			ComboBox_SetCurSel(yarn_type_combo_hwnd, sm->m_current_yarn_type);
		}
	}

	ThunderLoomMtlDlgProc(void) { sm = NULL; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg,
			WPARAM wParam, LPARAM lParam) {
		int id = LOWORD(wParam);
		switch (msg) 
		{
			case WM_INITDIALOG:{ 
				m_hWnd = hWnd;
				directory[0] = 0;
				update_yarn_type_combo();
				break;
            }
			case WM_DESTROY:
				break;
			case WM_COMMAND:
			{
				switch (LOWORD(wParam)) {
				case IDC_WIFFILE_BUTTON: {
					switch (HIWORD(wParam)) {
					case BN_CLICKED: {
						wchar_t *filters =
							L"Weave Files | *.WIF\0*.WIF\0"
							L"All Files | *.*\0*.*\0"
						;
						wchar_t buffer[512] = { 0 };
						OPENFILENAME openfilename = { 0 };
						openfilename.lStructSize = sizeof(OPENFILENAME);
						openfilename.hwndOwner = hWnd;
						openfilename.lpstrFilter = filters;
						openfilename.lpstrFile = buffer;
						openfilename.nMaxFile = 512;
						openfilename.lpstrInitialDir = directory;
						openfilename.lpstrTitle = L"Open weaving draft";
						if (GetOpenFileName(&openfilename)) {
							wcFreeWeavePattern(&(sm->m_weave_parameters));
							//NOTE(Vidar):Set default parameters...
							sm->m_weave_parameters.uscale = 1.0f;
							sm->m_weave_parameters.vscale = 1.0f;
							sm->m_weave_parameters.realworld_uv = 0;
							wcWeavePatternFromFile_wchar(
								&(sm->m_weave_parameters), buffer);
							Pattern *pattern = sm->m_weave_parameters.pattern;
							if(pattern) {
								for(int i=0;i<pattern->num_yarn_types;i++) {
									YarnType *yarn_type = pattern->yarn_types+i;
#define YARN_TYPE_PARAM(param) yarn_type->param = default_yarn_type.param;
									YARN_TYPE_PARAMETERS
								}
								for(int i=0;i<10;i++) {
									sm->pblock->SetValue(texmaps_diffuse, 0, NULL, i);
								}
							}
							IParamBlock2 *params = map->GetParamBlock();
                            sm->m_current_yarn_type = 0;
							update_yarn_type_combo();
							UpdateYarnTypeParameters(0, params,
								&(sm->m_weave_parameters),t);
							map->Invalidate();
						}
						break;
					}
					}
					break;
				}

				case IDC_TEX_DIFFUSE_BUTTON: {
					switch (HIWORD(wParam)) {
					case BN_CLICKED: {
#define BROWSE_MAPSONLY		(1<<1)
						//open material browser and ask for map
						BOOL newMat, cancel;
						MtlBase *mtlBase = GetCOREInterface()->DoMaterialBrowseDlg(m_hWnd, BROWSE_MAPSONLY, newMat, cancel);
						DBOUT("newMat: " << newMat << "cancel: " << cancel);
					
						if(!cancel) {
							DbgAssert((mtlBase == NULL) || ((mtlBase->SuperClassID() == TEXMAP_CLASS_ID)));
							Texmap* texmap = static_cast<Texmap*>(mtlBase);

							//TODO: set and update button
							MSTR s;
							texmap->GetClassName(s);
							HWND hCtrl = GetDlgItem( hWnd, IDC_TEX_DIFFUSE_BUTTON );
							ICustButton *button = GetICustButton(hCtrl);
							std::wostringstream str;
							str << "Map (" << s << ")";
							button->SetText(str.str().c_str());

							texmap->CreateParamDlg(sm->m_hwMtlEdit, sm->m_imp);

							//SetShader(texmap);
							//Update();
						}

						break;
					}
					}
					break;
				}
				}
				break;
		}

		}
		return FALSE;
	}
	void DeleteThis() {}
	void SetThing(ReferenceTarget *m)
	{
		sm = (ThunderLoomMtl*)m;
		update_yarn_type_combo();
	}
};
static ThunderLoomMtlDlgProc dlgProc;

/*===========================================================================*\
 |	Paramblock2 Descriptor
\*===========================================================================*/

//Set up paramblock to handle storing values and managing ui elements for us
static ParamBlockDesc2 thunder_loom_param_blk_desc(
    mtl_params, _T("Test mtl params"), 0,
    &thunderLoomDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, 0,
    //rollout
    IDD_BLENDMTL, IDS_PARAMETERS, 0, 0, new ThunderLoomMtlDlgProc(), 
    // pattern and geometry
    yrn_color, _FT("color"), TYPE_RGBA, P_ANIMATABLE, 0,
        p_default, Point3(0.3f,0.3f,0.3f),
        p_ui, TYPE_COLORSWATCH,IDC_YARNCOLOR_SWATCH,
    PB_END,
    mtl_yarn_type, _FT("yarn_type"), TYPE_INT, P_ANIMATABLE, 0,
        p_default, 0,
        p_ui, TYPE_INT_COMBOBOX, IDC_YARNTYPE_COMBO, 0,
    PB_END,
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
    yrn_umax, _FT("bend"), TYPE_FLOAT, P_ANIMATABLE, 0,
        p_default, 0.5,
        p_range, 0.0f, 1.f,
        p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_UMAX_EDIT, IDC_UMAX_SPIN, 0.1f,
    PB_END,
    yrn_psi, _FT("twist"), TYPE_FLOAT, P_ANIMATABLE, 0,
        p_default, 0.5,
        p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_PSI_EDIT, IDC_PSI_SPIN, 0.1f,
    PB_END,
    //Lighting params
    yrn_specular_strength, _FT("specular"), TYPE_FLOAT, P_ANIMATABLE, 0,
        p_default,		1.f,
        p_range,		0.0f, 1.f,
        p_ui,			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SPECULAR_EDIT, IDC_SPECULAR_SPIN, 0.1f,
    p_end,
    yrn_delta_x, _FT("highligtWidth"), TYPE_FLOAT, P_ANIMATABLE, 0,
        p_default, 0.5f,
        p_range, 0.0f, 1.f,
        p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_DELTAX_EDIT, IDC_DELTAX_SPIN, 0.1f,
    PB_END,
    yrn_alpha, _FT("alpha"), TYPE_FLOAT, P_ANIMATABLE, 0,
        p_default, 0.05,
        p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_ALPHA_EDIT, IDC_ALPHA_SPIN, 0.1f,
    PB_END,
    yrn_beta, _FT("beta"), TYPE_FLOAT, P_ANIMATABLE, 0,
        p_default, 4.f,
        p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_BETA_EDIT, IDC_BETA_SPIN, 0.1f,
    PB_END,
	//Let paramblocks handle ui for texmaps, this gets updated with the texmap for the current yarn type
  // mtl_texmap_diffuse, _T("diffuseMap"), TYPE_TEXMAP, 0, 0, 
  //      p_ui, TYPE_TEXMAPBUTTON, IDC_TEX_DIFFUSE_BUTTON,
  //      p_subtexno, 0,
  //  PB_END,
    texmaps_diffuse, _T("diffuseMapList"), TYPE_TEXMAP_TAB, 10, P_VARIABLE_SIZE, 0, //Should we use P_OWNERS_REF and P_VARIABLE_SIZE?
		//no ui, for the array
    PB_END,
PB_END
);									 


/*===========================================================================*\
 |	Constructor and Reset systems
 |  Ask the ClassDesc2 to make the AUTO_CONSTRUCT paramblocks and wire them in
\*===========================================================================*/

void ThunderLoomMtl::Reset() {
	m_weave_parameters.pattern = 0;
	ivalid.SetEmpty();
	thunderLoomDesc.Reset(this);
}

ThunderLoomMtl::ThunderLoomMtl(BOOL loading) {
	pblock=NULL;
	ivalid.SetEmpty();
	thunderLoomDesc.MakeAutoParamBlocks(this);
	Reset();
}

ParamDlg* ThunderLoomMtl::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp) {
	m_hwMtlEdit = hwMtlEdit;
	m_imp = imp;
	IAutoMParamDlg* masterDlg
		= thunderLoomDesc.CreateParamDlgs(hwMtlEdit, imp, this);
	DBOUT(masterDlg->NumDlgs());
	masterDlg->SetThing(this);
	return masterDlg;
}

BOOL ThunderLoomMtl::SetDlgThing(ParamDlg* dlg) {
	return FALSE;
}

Interval ThunderLoomMtl::Validity(TimeValue t) {
	Interval temp=FOREVER;
	pblock->GetValidity(t, temp);
	//Update(t, temp);

	//TODO(Peter): is validity okay? 
	// do we need to loop through each texmap and check fro validity there to?
    return temp;
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
		if (hTarget == pblock) {
			if (m_weave_parameters.pattern) {
				ParamID changing_param = pblock->LastNotifyParamID();
				thunder_loom_param_blk_desc.InvalidateUI(changing_param);
				YarnType *yarn_type = m_weave_parameters.pattern->yarn_types +
					m_current_yarn_type;

				switch (changing_param) {
					//TODO(Vidar): Not sure about the TimeValue...
				case mtl_yarn_type:
				{
					int sel;
					pblock->GetValue(mtl_yarn_type, 0, sel, ivalid);
					m_current_yarn_type = sel;
					UpdateYarnTypeParameters(sel, pblock,
						&m_weave_parameters, 0);
					break;
				}
				case yrn_color:
				{
					Color c;
					pblock->GetValue(yrn_color, 0, c,
						ivalid);
					float * col = m_weave_parameters.pattern->
						yarn_types[m_current_yarn_type].color;
					col[0] = c.r;
					col[1] = c.g;
					col[2] = c.b;
					break;
				}
				case mtl_realworld:
				{
					int realworld;
					pblock->GetValue(mtl_realworld, 0, realworld, ivalid);
					m_weave_parameters.realworld_uv = realworld;
					break;
				}
				case mtl_uscale:
				{
					pblock->GetValue(mtl_uscale, 0, m_weave_parameters.uscale,
						ivalid);
					break;
				}
				case mtl_vscale:
				{
					pblock->GetValue(mtl_vscale, 0, m_weave_parameters.vscale,
						ivalid);
					break;
				}

				//texmaps
				case mtl_texmap_diffuse:
				{
					Texmap *tex;
					pblock->GetValue(mtl_texmap_diffuse, 0, tex, ivalid);
					pblock->SetValue(texmaps_diffuse, 0, tex, m_current_yarn_type);
					thunder_loom_param_blk_desc.InvalidateUI(mtl_texmap_diffuse);
					break;
				}

				//yarn params
#define YARN_TYPE_PARAM(param) case yrn_##param: pblock->GetValue(yrn_##param,0,\
					yarn_type->param,ivalid); break;
				YARN_TYPE_PARAMETERS
				}

			}
			break;
		}
	}
	return(REF_SUCCEED);
}

//texmaps

Texmap* ThunderLoomMtl::GetSubTexmap(int i) {
	//TODO(Peter): Update this!
	DBOUT( "GetSubtex i: " << i );
	if (i == 0) {
		return pblock->GetTexmap(mtl_texmap_diffuse);
	}

	return NULL;
}

void ThunderLoomMtl::SetSubTexmap(int i, Texmap* m) {
	//TODO(Peter): Update this!
	//currently only 1 texmap. id == 0
	DBOUT( "SetSubtex i: " << i );
	if (i == 0) {
		pblock->SetValue(mtl_texmap_diffuse, 0, m);
	}
}

TSTR ThunderLoomMtl::GetSubTexmapSlotName(int i) {
	if (i == 0) {
		return L"Base";
	}

	return L"";
}

TSTR ThunderLoomMtl::GetSubTexmapTVName(int i) {
	return GetSubTexmapSlotName(i);
}

/*===========================================================================*\
 |	Standard IO
\*===========================================================================*/

#define MTL_HDR_CHUNK 0x4000
#define YARN_TYPE_CHUNK 0x0200

IOResult ThunderLoomMtl::Save(ISave *isave) { 
	IOResult res;
	isave->BeginChunk(MTL_HDR_CHUNK);
	res = MtlBase::Save(isave);
	if (res!=IO_OK) return res;
	isave->EndChunk();
	isave->BeginChunk(YARN_TYPE_CHUNK);
	//NOTE(Vidar):Save yarn types
	ULONG nb;
	isave->Write((unsigned char*)&m_weave_parameters,
		sizeof(wcWeaveParameters), &nb);
	Pattern *pattern = m_weave_parameters.pattern;
	isave->Write((unsigned char*)pattern, sizeof(Pattern), &nb);
	isave->Write((unsigned char*)pattern->yarn_types,
		sizeof(YarnType)*pattern->num_yarn_types, &nb);
	isave->Write((unsigned char*)pattern->entries,
		sizeof(PatternEntry)*m_weave_parameters.pattern_width
		*m_weave_parameters.pattern_height, &nb);
    isave->EndChunk();
	return IO_OK;
}	

IOResult ThunderLoomMtl::Load(ILoad *iload) { 
	IOResult res;
	int id;
    unsigned long n_read = 0;
	ULONG nb;
	while (IO_OK==(res=iload->OpenChunk())) {
		switch(id = iload->CurChunkID())  {
			case MTL_HDR_CHUNK:
				res = MtlBase::Load(iload);
				break;
            case YARN_TYPE_CHUNK:
			{
				//NOTE(Vidar):Load m_weave_parameters
				int num_yarn_types;
				wcWeaveParameters params;
				iload->Read((unsigned char*)&params,
					sizeof(wcWeaveParameters), &nb);
				Pattern *pattern = (Pattern*)calloc(1, sizeof(Pattern));
				iload->Read((unsigned char*)pattern,
					sizeof(Pattern), &nb);
				pattern->yarn_types = (YarnType*)calloc(pattern->num_yarn_types,
					sizeof(YarnType));
				iload->Read((unsigned char*)pattern->yarn_types,
					pattern->num_yarn_types * sizeof(YarnType), &nb);
				int num_entries = params.pattern_width * params.pattern_height;
				pattern->entries = (PatternEntry*)calloc(num_entries,
					sizeof(PatternEntry));
				iload->Read((unsigned char*)pattern->entries,
					num_entries * sizeof(PatternEntry), &nb);
				params.pattern = pattern;
				m_weave_parameters = params;
				break;
			}
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
	//TODO(Peter): Verify this!, call update on textures
	//if( pblock->GetTexmap(mtl_warpvar) )
		//	pblock->GetTexmap(mtl_warpvar)->Update(t, ivalid);
	if (!ivalid.InInterval(t)) {
		ivalid.SetInfinite();
	}
	valid &= ivalid;
}

/*===========================================================================*\
 |	Determine the characteristics of the material  (required for Mtl class)
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

/*===========================================================================*\
 |	Render intialization and deinitialization (From VUtils::VRenderMtl)
\*===========================================================================*/

void ThunderLoomMtl::renderBegin(TimeValue t, VR::VRayRenderer *vray) {
	ivalid.SetInfinite();

#ifdef DYNAMIC
	unload_dlls();
	EvalDiffuseFunc  = (EVALDIFFUSEFUNC) get_dynamic_func("eval_diffuse" );
	EvalSpecularFunc = (EVALSPECULARFUNC)get_dynamic_func("eval_specular");
#endif

	//default values for the time being.
	//these paramters will be removed later, is the plan
	m_weave_parameters.intensity_fineness = 0.f;
	m_weave_parameters.yarnvar_amplitude = 0.f;
	m_weave_parameters.yarnvar_xscale = 1.f;
	m_weave_parameters.yarnvar_yscale = 1.f;
	m_weave_parameters.yarnvar_persistance = 1.f;
	m_weave_parameters.yarnvar_octaves = 1;

	wcFinalizeWeaveParameters(&m_weave_parameters);

	const VR::VRaySequenceData &sdata=vray->getSequenceData();
	bsdfPool.init(sdata.maxRenderThreads);
}

void ThunderLoomMtl::renderEnd(VR::VRayRenderer *vray) {

	bsdfPool.freeMem();
	renderChannels.freeMem();
}

VR::BSDFSampler* ThunderLoomMtl::newBSDF(const VR::VRayContext &rc, VR::VRenderMtlFlags flags) {
		//TODO(Peter): Have mapping stuff here or in the BRDF?
		//it is 3dsmax specific
		//This is just a test!	
		float variation = 1.f;
		Texmap *tex = pblock->GetTexmap(mtl_texmap_diffuse);

	MyBlinnBSDF *bsdf=bsdfPool.newBRDF(rc);
	if (!bsdf) return NULL;
    bsdf->init(rc, &m_weave_parameters, tex);
	return bsdf;
}

void ThunderLoomMtl::deleteBSDF(const VR::VRayContext &rc, VR::BSDFSampler *b) {
	if (!b) return;
	MyBlinnBSDF *bsdf=static_cast<MyBlinnBSDF*>(b);
	bsdfPool.deleteBRDF(rc, bsdf);
}

void ThunderLoomMtl::addRenderChannel(int index) {
	renderChannels+=index;
}

VR::VRayVolume* ThunderLoomMtl::getVolume(const VR::VRayContext &rc) {
	return NULL;
}

/*===========================================================================*\
 |	Actual shading takes place (From BaseMTl)
\*===========================================================================*/

void ThunderLoomMtl::Shade(ShadeContext &sc) {
	if (sc.ClassID()==VRAYCONTEXT_CLASS_ID) {

		//shade() creates brdf, shades and deletes the brdf
		//it does this using the corresponding methods that have
		//been implemented from VUtils::VRenderMtl
		//newsBSDF, getVolume and deleteBSDF
		shade(static_cast<VR::VRayInterface&>(sc), gbufID);
	} 
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