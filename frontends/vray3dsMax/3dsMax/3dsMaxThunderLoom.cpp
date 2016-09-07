//3dsMaxThunderLoom.cpp
// This file sets up and registers the 3dsMax plugin.

//The paramblock version
const int PLUGIN_VERSION_HIGH=1;
const int PLUGIN_VERSION_LOW=00;
//This is the plugin version * 100
const int PLUGIN_VERSION= PLUGIN_VERSION_HIGH*100 + PLUGIN_VERSION_LOW;

// Check http://docs.autodesk.com/3DSMAX/16/ENU/3ds-Max-SDK-Programmer-Guide/index.html?url=files/GUID-F35959BB-2660-492F-B082-56304C70293A.htm,topicNumber=d30e52807
// When adding new parameters

#include "Eval.h"

#include "max.h"

#include "vrayinterface.h" //BSDFsampler amongst others..
#include "vrender_unicode.h"

#include "3dsMaxThunderLoom.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include "helper.h"

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

struct YarnTypeDlgProcData
{
    ThunderLoomMtl *sm;
    int yarn_type;
};

INT_PTR YarnTypeDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class PatternRolloutDlgProc: public ParamMap2UserDlgProc{
public:
	IParamMap *pmap;
	ThunderLoomMtl *sm;
	wchar_t directory[512];
	HWND *rollups;
	int num_rollups;

	void update_yarn_type_rollups()
	{
		if(sm && sm->m_i_mtl_params){
			for(int i=0; i<num_rollups; i++){
				sm->m_i_mtl_params->DeleteRollupPage(rollups[i]);
			}
		}
		free(rollups);
		if(sm && sm->m_weave_parameters.pattern && sm->m_i_mtl_params){
			num_rollups=sm->m_weave_parameters.pattern->num_yarn_types;
			rollups=(HWND*)calloc(num_rollups,sizeof(HWND));
			if(sm && sm->m_weave_parameters.pattern){
				//NOTE(Vidar): Create yarn type rollups
				for(int i=0; i<num_rollups; i++){
					YarnTypeDlgProcData *data=(YarnTypeDlgProcData*)
						calloc(1,sizeof(YarnTypeDlgProcData));
					data->sm=sm;
					data->yarn_type=i;
					bool open=sm->m_yarn_type_rollup_open[i];
					if(i==0){
						//NOTE(Vidar): Common yarn rollout
						rollups[0]=sm->m_i_mtl_params->AddRollupPage(
							hInstance,MAKEINTRESOURCE(IDD_YARN),YarnTypeDlgProc,
							L"Common Yarn Settings",
							(LPARAM)data,open ? 0 : APPENDROLL_CLOSED);
					} else{
						wchar_t buffer[128];
						wsprintf(buffer,L"Yarn type %d",i);
						rollups[i]=sm->m_i_mtl_params->AddRollupPage(hInstance,
							MAKEINTRESOURCE(IDD_YARN_TYPE),YarnTypeDlgProc,buffer,
							(LPARAM)data,open ? 0 : APPENDROLL_CLOSED);
					}
				}
			}
		} else{
			num_rollups=0;
			rollups=0;
		}
	}

	PatternRolloutDlgProc(void)
	{
		rollups=0;
		num_rollups=0;
		sm=NULL;
		directory[0]=0;
	}

	INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,
			WPARAM wParam,LPARAM lParam)
	{
		int id=LOWORD(wParam);
		switch(msg){
			case WM_INITDIALOG:
			{
				update_yarn_type_rollups();
				wchar_t buffer[256];
				swprintf(buffer,L"Thunder Loom version %d.%02d",PLUGIN_VERSION_HIGH,
					PLUGIN_VERSION_LOW);
				SetWindowText(GetDlgItem(hWnd,IDC_VERSION),buffer);
				break;
			}
			case WM_DESTROY:
				for(int i=0;i<num_rollups;i++){
					bool open=sm->m_i_mtl_params->IsRollupPanelOpen(rollups[i]);
					sm->m_yarn_type_rollup_open[i]=open;
				}
				if(rollups){
					free(rollups);
				}
				rollups=0;
				num_rollups=0;
				break;
			case WM_COMMAND:
			{
				switch(LOWORD(wParam)){
					case IDC_WIFFILE_BUTTON:
					{
						switch(HIWORD(wParam)){
							case BN_CLICKED:
							{
								wchar_t *filters=
									L"Weave Files | *.WIF\0*.WIF\0"
									L"All Files | *.*\0*.*\0"
									;
								wchar_t buffer[512]={0};
								OPENFILENAME openfilename={0};
								openfilename.lStructSize=sizeof(OPENFILENAME);
								openfilename.hwndOwner=hWnd;
								openfilename.lpstrFilter=filters;
								openfilename.lpstrFile=buffer;
								openfilename.nMaxFile=512;
								openfilename.lpstrInitialDir=directory;
								openfilename.lpstrTitle=L"Open weaving draft";
								if(GetOpenFileName(&openfilename)){
									IParamBlock2 *params=map->GetParamBlock();
									wcFreeWeavePattern(&(sm->m_weave_parameters));
									wcWeavePatternFromFile_wchar(
										&(sm->m_weave_parameters),buffer);
									//NOTE(Vidar):Set parameters...
									int realworld;
									sm->pblock->GetValue(mtl_realworld,0,realworld,
										sm->ivalid);
									sm->m_weave_parameters.realworld_uv=realworld;
									sm->pblock->GetValue(mtl_uscale,0,
										sm->m_weave_parameters.uscale,
										sm->ivalid);
									sm->pblock->GetValue(mtl_vscale,0,
										sm->m_weave_parameters.vscale,
										sm->ivalid);
									Pattern *pattern=sm->m_weave_parameters.pattern;
									sm->m_yarn_type_rollup_open[0]=1;
									for(int i=1;i<pattern->num_yarn_types;i++){
										sm->m_yarn_type_rollup_open[i]=0;
									}
									if(pattern){
										//TODO(Peter): Test this with more complicate wif files!
										//Set count for subtexmaps
										sm->pblock->SetCount(texmaps,
											pattern->num_yarn_types*
											NUMBER_OF_YRN_TEXMAPS);
									}
									map->Invalidate();
									update_yarn_type_rollups();
									//NOTE(Vidar): Hack to update the preview ball
									sm->pblock->SetValue(mtl_dummy,0,0.5f);
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
	void DeleteThis()
	{
		if(rollups){
			free(rollups);
		}
		rollups=0;
		num_rollups=0;
		sm=NULL;
	}
	void SetThing(ReferenceTarget *m)
	{
		sm=(ThunderLoomMtl*)m;
		update_yarn_type_rollups();
	}
};


/*===========================================================================*\
 |	Paramblock2 Descriptor
\*===========================================================================*/

enum {rollout_pattern, rollout_yarn_type};

static ParamBlockDesc2 thunder_loom_param_blk_desc(
    mtl_params, _T("Test mtl params"), 0,
    &thunderLoomDesc, P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP + P_VERSION,
	PLUGIN_VERSION, 0,
    //rollouts
	1,
	//2,
    rollout_pattern,   IDD_BLENDMTL, IDS_PATTERN, 0, 0,
		new PatternRolloutDlgProc(), 
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
    mtl_dummy, _FT("dummy"), TYPE_FLOAT, P_ANIMATABLE, 0,
        p_default, 1.f,
    PB_END,
    mtl_realworld, _FT("realworld"), TYPE_BOOL, 0, 0,
        p_default, FALSE,
        p_ui, rollout_pattern, TYPE_SINGLECHEKBOX, IDC_REALWORLD_CHECK,
    PB_END,
    texmaps, _T("yarnmaplist"), TYPE_TEXMAP_TAB, 0, P_VARIABLE_SIZE, 0,
		//no ui, for the array
		//handled through Proc
    PB_END,
PB_END
);									 

/*//Set up paramblock to handle storing values and managing ui elements for us
static ParamBlockDesc2 thunder_loom_param_blk_desc(
    mtl_params, _T("Test mtl params"), 0,
    &thunderLoomDesc, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION,
	PLUGIN_VERSION,
	1,
    rollout_pattern,   IDD_BLENDMTL, IDS_PATTERN, 0, 0,
		new PatternRolloutDlgProc(), 
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
    mtl_dummy, _FT("dummy"), TYPE_FLOAT, P_ANIMATABLE, 0,
        p_default, 1.f,
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
    texmaps, _T("yarnmaplist"), TYPE_TEXMAP_TAB, 0, P_VARIABLE_SIZE, 0,
		//no ui, for the array
		//handled through Proc
    PB_END,
PB_END
);									 
*/

INT_PTR YarnTypeDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int id = LOWORD(wParam);
    switch (msg) 
    {
        case WM_INITDIALOG:{ 
            YarnTypeDlgProcData *data = (YarnTypeDlgProcData*)lParam;
            SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)data);

            wcWeaveParameters params = data->sm->m_weave_parameters;
			if(params.pattern){
				YarnType yarn_type=params.pattern->yarn_types[data->yarn_type];
				//NOTE(Vidar): Setup spinners
				#define YARN_TYPE_PARAM(A,B){\
					HWND spinner_hwnd = GetDlgItem(hWnd, IDC_##B##_SPIN);\
					HWND edit_hwnd = GetDlgItem(hWnd, IDC_##B##_EDIT);\
					if(spinner_hwnd && edit_hwnd){\
						ISpinnerControl * s=GetISpinner(spinner_hwnd);\
						s->LinkToEdit(edit_hwnd,EDITTYPE_FLOAT);\
						s->SetLimits(0.f,1.f,FALSE);\
						s->SetScale(0.01f);\
						s->SetValue(yarn_type.A,FALSE);\
						ReleaseISpinner(s);\
						if(data->yarn_type>=0){\
							uint8_t enabled=yarn_type.A##_enabled;\
							Button_SetCheck(\
								GetDlgItem(hWnd,IDC_##B##_OVERRIDE),enabled);\
							EnableWindow(spinner_hwnd,enabled);\
							EnableWindow(edit_hwnd,enabled);\
						}\
					}\
				}
				YARN_TYPE_PARAMETERS
				#undef YARN_TYPE_PARAM

				for(int i=0; i<NUMBER_OF_YRN_TEXMAPS; i++){
					int subtexmap_id=
						NUMBER_OF_FIXED_TEXMAPS+
						data->yarn_type*
						NUMBER_OF_YRN_TEXMAPS+i;
					Texmap *texmap=data->sm->GetSubTexmap(subtexmap_id);
					wchar_t *caption=L"";
					if(texmap){
						caption=L"M";
					}
					HWND btn_hwnd = GetDlgItem(hWnd,texmapBtnIDCs[i]);
					SetWindowText(btn_hwnd,caption);
				}

				//NOTE(Vidar): Setup color picker
					HWND swatch_hwnd=GetDlgItem(hWnd,IDC_YARNCOLOR_SWATCH);
				IColorSwatch *swatch=GetIColorSwatch(swatch_hwnd);
				Color col(yarn_type.color[0],yarn_type.color[1],
					yarn_type.color[2]);
				swatch->SetColor(col);
				ReleaseIColorSwatch(swatch);
				if(data->yarn_type>=0){
					uint8_t enabled=yarn_type.color_enabled;
					Button_SetCheck(GetDlgItem(hWnd,IDC_COLOR_OVERRIDE),enabled);
					EnableWindow(swatch_hwnd,enabled);
				}
			}
            break;
        }

		#define GET_YARN_TYPE\
			YarnTypeDlgProcData *data = (YarnTypeDlgProcData*)\
				GetWindowLongPtr(hWnd,GWLP_USERDATA);\
			YarnType * yarn_type = &data->sm->m_weave_parameters.pattern\
					->yarn_types[data->yarn_type];
			 //NOTE(Vidar): Hack to update the preview ball
		#define UPDATE_BALL\
            data->sm->pblock->SetValue(mtl_dummy,0,0.5f);

        case WM_DESTROY:
		{
			break;
		}

	    //NOTE(Vidar): Update checkbox
        case WM_COMMAND: {
            switch(HIWORD(wParam)){
                case BN_CLICKED:{
					GET_YARN_TYPE
                    switch(id){

						#define YARN_TYPE_PARAM(A,B) case IDC_##B##_OVERRIDE:{\
							HWND spin_hwnd = GetDlgItem(hWnd,IDC_##B##_SPIN);\
							HWND edit_hwnd = GetDlgItem(hWnd,IDC_##B##_EDIT);\
							bool check = Button_GetCheck((HWND)lParam);\
							EnableWindow(spin_hwnd,check);\
							EnableWindow(edit_hwnd,check);\
							yarn_type->A##_enabled = check;\
							break;\
						}
                        YARN_TYPE_PARAMETERS
						#undef YARN_TYPE_PARAM

                        case IDC_COLOR_OVERRIDE:{
                            HWND swatch_hwnd =
                                GetDlgItem(hWnd,IDC_YARNCOLOR_SWATCH);
                            bool check = Button_GetCheck((HWND)lParam);
                            EnableWindow(swatch_hwnd,check);
                            yarn_type->color_enabled = check;
                            break;
                        }
						//NOTE(Vidar): Texmap buttons
						default: {
							if(data->sm->m_weave_parameters.pattern) {
								for(int i = 0; i < NUMBER_OF_YRN_TEXMAPS; i++) {
									if (LOWORD(wParam) == texmapBtnIDCs[i]
										&& HIWORD(wParam) == BN_CLICKED){
										//set mtl!
										int subtexmap_id=
											NUMBER_OF_FIXED_TEXMAPS+
											data->yarn_type*
											NUMBER_OF_YRN_TEXMAPS+i;
										//User pressed Texmap button for ith submap
										PostMessage(data->sm->m_hwMtlEdit,
											WM_TEXMAP_BUTTON,subtexmap_id,
											(LPARAM)data->sm);
									}
								}
							}
						}
                    }
					UPDATE_BALL;
                    break;
                }
            }
        }
	    //NOTE(Vidar): Update float parameters
        case CC_SPINNER_CHANGE: {
			GET_YARN_TYPE
            ISpinnerControl *spinner = (ISpinnerControl*)lParam;
            switch(id){
				#define YARN_TYPE_PARAM(yarntype_name,idc_name)\
					case IDC_##idc_name##_SPIN:\
					yarn_type->yarntype_name = spinner->GetFVal(); break;
                YARN_TYPE_PARAMETERS
				#undef YARN_TYPE_PARAM
            }
			UPDATE_BALL
            break;
        }
	    //NOTE(Vidar): Update color parameters
        case CC_COLOR_CHANGE:{
			GET_YARN_TYPE
            IColorSwatch *swatch = (IColorSwatch*)lParam;
            COLORREF col = swatch->GetColor();
            yarn_type->color[0] = (float)GetRValue(col)/255.f;
            yarn_type->color[1] = (float)GetGValue(col)/255.f;
            yarn_type->color[2] = (float)GetBValue(col)/255.f;
			UPDATE_BALL
            break;
         }
		case WM_CONTEXTMENU:
		{
			GET_YARN_TYPE
			ThunderLoomMtl *sm=data->sm;
			// Note(Peter): Allow for right click context menu when rightclicking 
			// yarn texmaps, much like the way when clicking regular texmaps buttons
			// handled by a paramblock. Is probably a a better way to get the same
			// menu without, cannot find how though, this will do for now.
			POINT ptScreen={LOWORD(lParam), HIWORD(lParam)};
			POINT ptClient=ptScreen;
			ScreenToClient(hWnd,&ptClient);
			HWND hWndChild=RealChildWindowFromPoint(hWnd,ptClient); //Get handle to clicked child window
			for(int i=0; i<NUMBER_OF_YRN_TEXMAPS; i++){
				HWND btn_hwnd=GetDlgItem(hWnd,texmapBtnIDCs[i]);
				if(hWndChild==btn_hwnd){
					//We right clicked on a texture button!
					int subtex_id=NUMBER_OF_FIXED_TEXMAPS+NUMBER_OF_YRN_TEXMAPS*data->yarn_type+i;
					Texmap *texmap=sm->GetSubTexmap(subtex_id);
					if(!texmap){
						break;
					}

					HMENU hMenu=CreatePopupMenu();
					LPCTSTR menuLabel=L"Clear\0";
					AppendMenu(hMenu,MF_STRING,IDR_MENU_TEX_CLEAR,menuLabel);
					int return_value=TrackPopupMenu(hMenu,TPM_LEFTALIGN+TPM_TOPALIGN+TPM_RETURNCMD+TPM_RIGHTBUTTON+TPM_NOANIMATION,ptScreen.x,ptScreen.y,0,hWnd,0);
					if(return_value==IDR_MENU_TEX_CLEAR){
						//We clicked on Clear item in context menu
						sm->SetSubTexmap(subtex_id,NULL);
						SetWindowText(btn_hwnd,L"");
						RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE);
					}
				}
			}
			UPDATE_BALL;
		}
    }
    return FALSE;
}

/*===========================================================================*\
 |	Constructor and Reset systems
 |  Ask the ClassDesc2 to make the AUTO_CONSTRUCT paramblocks and wire them in
\*===========================================================================*/

void ThunderLoomMtl::Reset() {
    m_i_mtl_params = 0;
	m_weave_parameters.pattern = 0;
	ivalid.SetEmpty();
	thunderLoomDesc.Reset(this);
}

ThunderLoomMtl::ThunderLoomMtl(BOOL loading) {
    m_i_mtl_params = 0;
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
    m_i_mtl_params = imp;
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

				//TODO(Vidar): Not sure about the TimeValue... using 0 for now
				switch(changing_param){
					case mtl_realworld:
					{
						int realworld;
						pblock->GetValue(mtl_realworld,0,realworld,ivalid);
						m_weave_parameters.realworld_uv=realworld;
						break;
					}
					case mtl_uscale:
					{
						pblock->GetValue(mtl_uscale,0,m_weave_parameters.uscale,
							ivalid);
						break;
					}
					case mtl_vscale:
					{
						pblock->GetValue(mtl_vscale,0,m_weave_parameters.vscale,
							ivalid);
						break;
					}
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
	if (NumSubTexmaps() > NUMBER_OF_FIXED_TEXMAPS && i < NumSubTexmaps()) {
		return pblock->GetTexmap(texmaps, 0, i-NUMBER_OF_FIXED_TEXMAPS);
	} else {
		return NULL;
	}
}

void ThunderLoomMtl::SetSubTexmap(int i, Texmap* m) {
	if (NumSubTexmaps() > NUMBER_OF_FIXED_TEXMAPS && i < NumSubTexmaps()) {
		pblock->SetValue(texmaps, 0, m, i-NUMBER_OF_FIXED_TEXMAPS);
	}
}

TSTR ThunderLoomMtl::GetSubTexmapSlotName(int i) {
	int yrntexmap_id = i - NUMBER_OF_FIXED_TEXMAPS;
	switch(yrntexmap_id % NUMBER_OF_YRN_TEXMAPS) {
#define YARN_TYPE_TEXMAP(param) case yrn_texmaps_##param: return L#param;
		YARN_TYPE_TEXMAP_PARAMETERS
#undef YARN_TYPE_TEXMAP
		default:
			return L"";
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

IOResult ThunderLoomMtl::Save(ISave *isave)
{
	IOResult res;
	isave->BeginChunk(MTL_HDR_CHUNK);
	res=MtlBase::Save(isave);
	if(res!=IO_OK) return res;
	isave->EndChunk();
	isave->BeginChunk(YARN_TYPE_CHUNK);
	//NOTE(Vidar):Save yarn types
	ULONG nb;
	isave->Write((unsigned char*)&PLUGIN_VERSION,
		sizeof(int),&nb);
	isave->Write((unsigned char*)&m_weave_parameters,
		sizeof(wcWeaveParameters),&nb);
	Pattern *pattern=m_weave_parameters.pattern;
	if(pattern){
		isave->Write((unsigned char*)pattern,sizeof(Pattern),&nb);
		isave->Write((unsigned char*)pattern->yarn_types,
			sizeof(YarnType)*pattern->num_yarn_types,&nb);
		isave->Write((unsigned char*)pattern->entries,
			sizeof(PatternEntry)*m_weave_parameters.pattern_width
			*m_weave_parameters.pattern_height,&nb);
	}
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
				int version;
				iload->Read((unsigned char*)&version,
					sizeof(int), &nb);
				//NOTE(Vidar):Load m_weave_parameters
				int num_yarn_types;
				wcWeaveParameters params;
				iload->Read((unsigned char*)&params,
					sizeof(wcWeaveParameters), &nb);
				if(params.pattern){
					Pattern *pattern=(Pattern*)calloc(1,sizeof(Pattern));
					iload->Read((unsigned char*)pattern,
						sizeof(Pattern),&nb);
					pattern->yarn_types=(YarnType*)calloc(pattern->num_yarn_types,
						sizeof(YarnType));
					iload->Read((unsigned char*)pattern->yarn_types,
						pattern->num_yarn_types*sizeof(YarnType),&nb);
					int num_entries=params.pattern_width * params.pattern_height;
					pattern->entries=(PatternEntry*)calloc(num_entries,
						sizeof(PatternEntry));
					iload->Read((unsigned char*)pattern->entries,
						num_entries*sizeof(PatternEntry),&nb);
					params.pattern = pattern;
				}
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
	//TODO(Vidar):Copy parameters...
	mnew->m_weave_parameters=m_weave_parameters;
	if(m_weave_parameters.pattern){
		Pattern *pattern;
		pattern=(Pattern*)calloc(1,sizeof(Pattern));
		*pattern=*m_weave_parameters.pattern;
		pattern->yarn_types=(YarnType*)calloc(pattern->num_yarn_types,
			sizeof(YarnType));
		memcpy(pattern->yarn_types,m_weave_parameters.pattern->yarn_types,
			pattern->num_yarn_types*sizeof(YarnType));
		int num_entries=m_weave_parameters.pattern_width *
			m_weave_parameters.pattern_height;
		pattern->entries=(PatternEntry*)calloc(num_entries,
			sizeof(PatternEntry));
		memcpy(pattern->entries,m_weave_parameters.pattern->entries,
			num_entries*sizeof(PatternEntry));
		mnew->m_weave_parameters.pattern=pattern;
	}
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

	//default values for the time being.
	//these paramters will be removed later, is the plan
	m_weave_parameters.yarnvar_amplitude = 0.f;
	m_weave_parameters.yarnvar_xscale = 1.f;
	m_weave_parameters.yarnvar_yscale = 1.f;
	m_weave_parameters.yarnvar_persistance = 1.f;
	m_weave_parameters.yarnvar_octaves = 1;

	wcFinalizeWeaveParameters(&m_weave_parameters);

	if(m_weave_parameters.pattern){
		for(int i=0;i<m_weave_parameters.pattern->num_yarn_types;i++){
			YarnType *yarn_type=&m_weave_parameters.pattern->yarn_types[i];
		#define YARN_TYPE_TEXMAP(param,A)\
			yarn_type->param##_texmap= GetSubTexmap(NUMBER_OF_FIXED_TEXMAPS+\
				NUMBER_OF_YRN_TEXMAPS*i +yrn_texmaps_##param);
			YARN_TYPE_TEXMAP_PARAMETERS
		#undef YARN_TYPE_TEXMAP
		}
	}

	const VR::VRaySequenceData &sdata=vray->getSequenceData();
	bsdfPool.init(sdata.maxRenderThreads);
}

void ThunderLoomMtl::renderEnd(VR::VRayRenderer *vray) {
	bsdfPool.freeMem();
	renderChannels.freeMem();
}

VR::BSDFSampler* ThunderLoomMtl::newBSDF(const VR::VRayContext &rc, VR::VRenderMtlFlags flags) {
	MyBlinnBSDF *bsdf=bsdfPool.newBRDF(rc);
	if (!bsdf) return NULL;
	bsdf->init(rc, &m_weave_parameters);
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