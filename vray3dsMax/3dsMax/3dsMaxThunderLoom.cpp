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
			//Extract a resource from the calling module's string table.
			static MSTR category(MaxSDK::GetResourceStringAsMSTR(hInst,
				IDS_3DSMAX_SME_MATERIALS_CATLABEL).Append(_T("\\V-Ray")));
			return category.data();
		}
#endif
		return _T("Materials\\V-Ray");
	}
	Bitmap* GetEntryThumbnail() const { return NULL; }
#endif
};
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
		pblock->SetValue(yrn_color, t, Point3(yarn_type.color[0],
			yarn_type.color[1], yarn_type.color[2]));
		
#define YARN_TYPE_PARAM(param) pblock->SetValue(yrn_##param, t,yarn_type.param);
		YARN_TYPE_PARAMETERS

		pblock->SetValue(mtl_uscale, t, weave_params->uscale);
		pblock->SetValue(mtl_vscale, t, weave_params->vscale);
		pblock->SetValue(mtl_intensity_fineness, t, weave_params->intensity_fineness);
			
	}
}

static void UpdateYarnTexmaps(int yarn_type_id, IParamBlock2 *pblock, HWND hWnd, TimeValue t)
{
	for (int i = yarn_type_id*NUMBER_OF_YRN_TEXMAPS;
		i < (yarn_type_id+1)*NUMBER_OF_YRN_TEXMAPS; i++) {
		int texmap_id = i % NUMBER_OF_YRN_TEXMAPS;
		Texmap *texmap = pblock->GetTexmap(texmaps, 0, i); //0 is main diffuse
		std::wostringstream str;

		if (texmap) {
			MSTR s; texmap->GetClassName(s);
			str << "Map (" << s << ")";
		} else {
			str << "None";
		}

		HWND hCtrl = GetDlgItem( hWnd, texmapBtnIDCs[texmap_id]);
		ICustButton *button = GetICustButton(hCtrl);
		button->SetText(str.str().c_str());
	}
}

class PatternRolloutDlgProc : public ParamMap2UserDlgProc {
public:
	IParamMap *pmap;
	ThunderLoomMtl *sm;
	wchar_t directory[512];

	PatternRolloutDlgProc(void)
	{
		sm = NULL;
		directory[0] = 0;
	}
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg,
			WPARAM wParam, LPARAM lParam) {
		int id = LOWORD(wParam);
		switch (msg) 
		{
			case WM_INITDIALOG:{ 
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
							IParamBlock2 *params = map->GetParamBlock();
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
								//TODO(Peter): Test this with more complicate wif files!
								//Set count for subtexmaps
								sm->pblock->SetCount(texmaps, pattern->num_yarn_types*NUMBER_OF_YRN_TEXMAPS);
							}
                            sm->m_current_yarn_type = 0;
							UpdateYarnTypeParameters(0, params,
								&(sm->m_weave_parameters),t);

							//NOTE(Vidar):Update the yarn rollout too...
							ParamDlg *yarn_dlg =
								params->GetMParamDlg()->GetDlg(0);
							yarn_dlg->SetThing(sm);
							map->Invalidate();
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
	}
};


//Here there is the option to add custom behaviour to certain messages for the yarn rollout.
class YarnRolloutDlgProc : public ParamMap2UserDlgProc {
public:
	IParamMap *pmap;
	ThunderLoomMtl *sm;
	HWND m_hWnd;

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

	YarnRolloutDlgProc(void) { sm = NULL; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg,
			WPARAM wParam, LPARAM lParam) {
		int id = LOWORD(wParam);
		switch (msg) 
		{
			case WM_INITDIALOG:{ 
				m_hWnd = hWnd;
				update_yarn_type_combo();
				break;
            }
			case WM_DESTROY:
				break;
			case WM_COMMAND:
			{
				switch (LOWORD(wParam)) {
					/*case IDC_WIFFILE_BUTTON: {
						//do something
						break;
					}*/
				
					default: {
						if(sm->m_weave_parameters.pattern) {
						for(int i = 0; i < NUMBER_OF_YRN_TEXMAPS; i++) {
							if (LOWORD(wParam) == texmapBtnIDCs[i] && HIWORD(wParam) == BN_CLICKED){
								//set mtl!
								int subtexmap_id = NUMBER_OF_FIXED_TEXMAPS + sm->m_current_yarn_type*NUMBER_OF_YRN_TEXMAPS + i;
								//User pressed Texmap button for ith submap
								PostMessage(sm->m_hwMtlEdit, WM_TEXMAP_BUTTON, subtexmap_id,(LPARAM)sm);
								DBOUT("Posted WM_TEXMAP_BUTTON, with texmap id " << subtexmap_id);
							}
						}
						}
					}
				}

			}
			case WM_CONTEXTMENU:
			{
				// Note(Peter): Allow for right click context menu when rightclicking 
				// yarn texmaps, much like the way when clicking regular texmaps buttons
				// handled by a paramblock. Is probably a a better way to get the same
				// menu without, cannot find how though, this will do for now.
				POINT ptScreen = {LOWORD(lParam), HIWORD(lParam)};
				POINT ptClient = ptScreen;
				ScreenToClient(hWnd, &ptClient);
				HWND hWndChild  = RealChildWindowFromPoint(hWnd, ptClient); //Get handle to clicked child window
				for(int i = 0; i < NUMBER_OF_YRN_TEXMAPS; i++) {
					if (hWndChild == GetDlgItem(hWnd, texmapBtnIDCs[i])){
						//We right clicked on a texture button!
						int subtex_id = NUMBER_OF_FIXED_TEXMAPS + NUMBER_OF_YRN_TEXMAPS*sm->m_current_yarn_type + i;
						Texmap *texmap = sm->GetSubTexmap(subtex_id);
						if (!texmap) {
							break;
						}

						HMENU hMenu = CreatePopupMenu();
						LPCTSTR menuLabel = L"Clear\0";
						AppendMenu(hMenu, MF_STRING, IDR_MENU_TEX_CLEAR, menuLabel);
						int return_value = TrackPopupMenu(hMenu, TPM_LEFTALIGN + TPM_TOPALIGN + TPM_RETURNCMD + TPM_RIGHTBUTTON + TPM_NOANIMATION, ptScreen.x, ptScreen.y, 0, hWnd, 0);
						if (return_value == IDR_MENU_TEX_CLEAR) {
							//We clicked on Clear item in context menu
							sm->SetSubTexmap(subtex_id,NULL);
							SetThing(sm);
						}
					}
				}
			}
		}
		return FALSE;
	}
	void DeleteThis() {}
	void SetThing(ReferenceTarget *m)
	{
		sm = (ThunderLoomMtl*)m;
		update_yarn_type_combo();
		if (sm->m_weave_parameters.pattern){
			//Update yarn texmaps here so that buttons have correct text
			//when returning from editing subtexmap.
			UpdateYarnTexmaps(sm->m_current_yarn_type, sm->pblock, m_hWnd, 0);
		}
	}
};

/*===========================================================================*\
 |	Paramblock2 Descriptor
\*===========================================================================*/

enum {rollout_pattern, rollout_yarn_type};

//Set up paramblock to handle storing values and managing ui elements for us
static ParamBlockDesc2 thunder_loom_param_blk_desc(
    mtl_params, _T("Test mtl params"), 0,
    &thunderLoomDesc, P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP, 0,
    //rollouts
	2,
    rollout_pattern,   IDD_BLENDMTL, IDS_PATTERN, 0, 0,
		new PatternRolloutDlgProc(), 
    rollout_yarn_type, IDD_YARN,     IDS_YARN, 0, 0,
		new YarnRolloutDlgProc(), 
    // pattern and geometry
    yrn_color, _FT("color"), TYPE_RGBA, P_ANIMATABLE, 0,
        p_default, Point3(0.3f,0.3f,0.3f),
        p_ui, rollout_yarn_type, TYPE_COLORSWATCH,IDC_YARNCOLOR_SWATCH,
    PB_END,
    mtl_yarn_type, _FT("yarn_type"), TYPE_INT, P_ANIMATABLE, 0,
        p_default, 0,
        p_ui, rollout_yarn_type, TYPE_INT_COMBOBOX, IDC_YARNTYPE_COMBO, 0,
    PB_END,
    mtl_uscale, _FT("uscale"), TYPE_FLOAT, P_ANIMATABLE, 0,
        p_default, 1.f,
        p_ui, rollout_pattern, TYPE_SPINNER, EDITTYPE_FLOAT,
			IDC_USCALE_EDIT, IDC_USCALE_SPIN, 0.1f,
    PB_END,
    mtl_vscale, _FT("vscale"), TYPE_FLOAT, P_ANIMATABLE, 0,
        p_default, 1.f,
        p_ui, rollout_pattern, TYPE_SPINNER, EDITTYPE_FLOAT,
			IDC_VSCALE_EDIT, IDC_VSCALE_SPIN, 0.1f,
    PB_END,
    mtl_realworld, _FT("realworld"), TYPE_BOOL, 0, 0,
        p_default, FALSE,
        p_ui, rollout_pattern, TYPE_SINGLECHEKBOX, IDC_REALWORLD_CHECK,
    PB_END,
    mtl_intensity_fineness, _FT("specularnoise"), TYPE_FLOAT, P_ANIMATABLE, 0,
        p_default, 0.f,
		p_range, 0.0f, 10.f,
		p_ui, rollout_pattern, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SPECULARNOISE_EDIT, IDC_SPECULARNOISE_SPIN, 0.1f,
    PB_END,
    yrn_umax, _FT("bend"), TYPE_FLOAT, P_ANIMATABLE, 0,
        p_default, 0.5,
        p_range, 0.0f, 1.f,
        p_ui, rollout_yarn_type, TYPE_SPINNER, EDITTYPE_FLOAT,
			IDC_UMAX_EDIT, IDC_UMAX_SPIN, 0.1f,
    PB_END,
    yrn_psi, _FT("twist"), TYPE_FLOAT, P_ANIMATABLE, 0,
        p_default, 0.5,
        p_ui, rollout_yarn_type, TYPE_SPINNER, EDITTYPE_FLOAT,
			IDC_PSI_EDIT, IDC_PSI_SPIN, 0.1f,
    PB_END,
    //Lighting params
    yrn_specular_strength, _FT("specular"), TYPE_FLOAT, P_ANIMATABLE, 0,
        p_default,		1.f,
        p_range,		0.0f, 1.f,
        p_ui, rollout_yarn_type, TYPE_SPINNER, EDITTYPE_FLOAT,
			IDC_SPECULAR_EDIT, IDC_SPECULAR_SPIN, 0.1f,
    PB_END,
    yrn_delta_x, _FT("highligtWidth"), TYPE_FLOAT, P_ANIMATABLE, 0,
        p_default, 0.5f,
        p_range, 0.0f, 1.f,
        p_ui, rollout_yarn_type, TYPE_SPINNER, EDITTYPE_FLOAT,
			IDC_DELTAX_EDIT, IDC_DELTAX_SPIN, 0.1f,
    PB_END,
    yrn_alpha, _FT("alpha"), TYPE_FLOAT, P_ANIMATABLE, 0,
        p_default, 0.05,
        p_ui, rollout_yarn_type, TYPE_SPINNER, EDITTYPE_FLOAT,
			IDC_ALPHA_EDIT, IDC_ALPHA_SPIN, 0.1f,
    PB_END,
    yrn_beta, _FT("beta"), TYPE_FLOAT, P_ANIMATABLE, 0,
        p_default, 4.f,
        p_ui, rollout_yarn_type, TYPE_SPINNER, EDITTYPE_FLOAT,
			IDC_BETA_EDIT, IDC_BETA_SPIN, 0.1f,
    PB_END,
	mtl_texmap_diffuse, _T("diffuseMap"), TYPE_TEXMAP, 0, 0, 
		p_subtexno, 0,
		p_ui, rollout_pattern, TYPE_TEXMAPBUTTON, IDC_TEX_DIFFUSE_BUTTON,
    PB_END,
	mtl_texmap_specular, _T("specularMap"), TYPE_TEXMAP, 0, 0, 
		p_subtexno, 1,
		p_ui, rollout_pattern, TYPE_TEXMAPBUTTON, IDC_TEX_SPECULAR_BUTTON,
    PB_END,
    texmaps, _T("diffuseMapList"), TYPE_TEXMAP_TAB, 0, P_VARIABLE_SIZE, 0,
		//no ui, for the array
		//handled through Proc
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

				//TODO(Vidar): Not sure about the TimeValue... using 0 for now
				switch (changing_param) {
				case mtl_yarn_type:
				{
					int sel;
					pblock->GetValue(mtl_yarn_type, 0, sel, ivalid);
					m_current_yarn_type = sel;
					//Select has changed.
					pblock->GetMParamDlg()->GetDlg(0)->SetThing(this);
					//map->Invalidate();
					// TODO(Peter): move UpdateYarnTypeParameters into setThing?
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
				case mtl_intensity_fineness:
				{
					pblock->GetValue(mtl_intensity_fineness, 0, m_weave_parameters.intensity_fineness,
						ivalid);
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

int ThunderLoomMtl::NumSubTexmaps() {
	if (this->m_weave_parameters.pattern) {
		return NUMBER_OF_FIXED_TEXMAPS + this->m_weave_parameters.pattern->num_yarn_types*NUMBER_OF_YRN_TEXMAPS;
	} else {
		return NUMBER_OF_FIXED_TEXMAPS;
	}
}

Texmap* ThunderLoomMtl::GetSubTexmap(int i) {
	switch (i)
	{
		case 0:
			return pblock->GetTexmap(mtl_texmap_diffuse);
			break;
		case 1:
			return pblock->GetTexmap(mtl_texmap_specular);
			break;
		default:
			if (NumSubTexmaps() > NUMBER_OF_FIXED_TEXMAPS && i < NumSubTexmaps()) {
				return pblock->GetTexmap(texmaps, 0, i-NUMBER_OF_FIXED_TEXMAPS);
			} else {
				return NULL;
			}
			break;
	}
}

void ThunderLoomMtl::SetSubTexmap(int i, Texmap* m) {
	switch (i)
	{
		case 0:
			pblock->SetValue(mtl_texmap_diffuse, 0, m);
			break;
		case 1:
			pblock->SetValue(mtl_texmap_specular, 0, m);
			break;
		default:
			if (NumSubTexmaps() > NUMBER_OF_FIXED_TEXMAPS && i < NumSubTexmaps()) {
				pblock->SetValue(texmaps, 0, m, i-NUMBER_OF_FIXED_TEXMAPS);
			}
			break;
	}
}

TSTR ThunderLoomMtl::GetSubTexmapSlotName(int i) {
	switch(i) {
		case 0: return L"Main Diffuse Map";
		case 1: return L"Main Specular Map";
	}

	//if not main texmap, then yarntexmap
	wchar_t *texstr;
	wchar_t str[80];
	wcscpy (str,L"Yarn ");
	int yrntexmap_id = i - NUMBER_OF_FIXED_TEXMAPS;
	switch(yrntexmap_id % NUMBER_OF_YRN_TEXMAPS) {
#define YARN_TYPE_TEXMAP(param) case yrn_texmaps_##param: texstr = L#param;break;
		YARN_TYPE_TEXMAP_PARAMETERS
		
		default:
			return L"";
	}
	wcscat(str,texstr);
	return str;
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
	m_weave_parameters.yarnvar_amplitude = 0.f;
	m_weave_parameters.yarnvar_xscale = 1.f;
	m_weave_parameters.yarnvar_yscale = 1.f;
	m_weave_parameters.yarnvar_persistance = 1.f;
	m_weave_parameters.yarnvar_octaves = 1;

	wcFinalizeWeaveParameters(&m_weave_parameters);

	//Gather texmap pointers for use in vray, gathering here to avoid including paramblock2.h in eval function.
	//Not gathering in newBSDF since that gets called for every sample.
	DBOUT("Gathering texmaps!")
	if (this->m_weave_parameters.pattern) {
		int ntexmaps = NUMBER_OF_FIXED_TEXMAPS + NUMBER_OF_YRN_TEXMAPS * this->m_weave_parameters.pattern->num_yarn_types;
		m_tmp_alltexmaps = (Texmap**)calloc(ntexmaps,sizeof(Texmap*));
		for (int i = 0; i < ntexmaps; i++) {
			m_tmp_alltexmaps[i] = GetSubTexmap(i);
		}
	}

	const VR::VRaySequenceData &sdata=vray->getSequenceData();
	bsdfPool.init(sdata.maxRenderThreads);
}

void ThunderLoomMtl::renderEnd(VR::VRayRenderer *vray) {
	if (this->m_weave_parameters.pattern) {
		free(m_tmp_alltexmaps);
	}
	bsdfPool.freeMem();
	renderChannels.freeMem();
}

VR::BSDFSampler* ThunderLoomMtl::newBSDF(const VR::VRayContext &rc, VR::VRenderMtlFlags flags) {
	MyBlinnBSDF *bsdf=bsdfPool.newBRDF(rc);
	if (!bsdf) return NULL;
	bsdf->init(rc, &m_weave_parameters, m_tmp_alltexmaps);
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