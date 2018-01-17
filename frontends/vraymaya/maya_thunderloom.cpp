// Check for 64-bit OS and define the Bits64_ macro required by Maya headers
//#include "defines.h"
//#ifdef X64
//#define Bits64_
//#endif

// Maya headers
#include <maya/MPxNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnMesh.h>
#include <maya/MFnMeshData.h>
#include <maya/MPointArray.h>
#include <maya/MPoint.h>
#include <maya/MFloatVector.h>
#include <maya/MDoubleArray.h>

// V-Ray headers
#include "vraybase.h"
#include "maya_thunderloom.h"
#include "vrayplugins.h"

#define TL_THUNDERLOOM_IMPLEMENTATION
#define TL_NO_TEXTURE_CALLBACKS
#include "../../src/thunderloom.h"

// VRayThunderLoom
// We have a registered Node ID Block of 0x0012bf80 - 0x0012bfbf for ThunderLoom.
MTypeId VRayThunderLoom::id(0x0012bf80);
MString VRayThunderLoom::apiType("VRayThunderLoomMtl");
MString VRayThunderLoom::classification("shader/surface/utility/:swatch/VRayMtlSwatchGen");

MObject VRayThunderLoom::m_filepath;
MObject VRayThunderLoom::m_uscale;
MObject VRayThunderLoom::m_vscale;
MObject VRayThunderLoom::m_uvrotation;
MObject VRayThunderLoom::m_bend;
MObject VRayThunderLoom::m_bend_on;
MObject VRayThunderLoom::m_yarnsize;
MObject VRayThunderLoom::m_yarnsize_on;
MObject VRayThunderLoom::m_twist;
MObject VRayThunderLoom::m_twist_on;
MObject VRayThunderLoom::m_alpha;
MObject VRayThunderLoom::m_alpha_on;
MObject VRayThunderLoom::m_beta;
MObject VRayThunderLoom::m_beta_on;
MObject VRayThunderLoom::m_specular_color;
MObject VRayThunderLoom::m_specular_color_on;
MObject VRayThunderLoom::m_specular_color_amount;
MObject VRayThunderLoom::m_specular_color_amount_on;
MObject VRayThunderLoom::m_specular_noise;
MObject VRayThunderLoom::m_specular_noise_on;
MObject VRayThunderLoom::m_highlight_width;
MObject VRayThunderLoom::m_highlight_width_on;
MObject VRayThunderLoom::m_diffuse_color;
MObject VRayThunderLoom::m_diffuse_color_on;
MObject VRayThunderLoom::m_diffuse_color_amount;
MObject VRayThunderLoom::m_diffuse_color_amount_on;

MObject VRayThunderLoom::m_outColor;
MObject VRayThunderLoom::m_outTransparency;
MObject VRayThunderLoom::m_outApiType;
MObject VRayThunderLoom::m_outApiClassification;

VRayThunderLoom::VRayThunderLoom() {
}

VRayThunderLoom::~VRayThunderLoom() {
}

void* VRayThunderLoom::creator() {
    return new VRayThunderLoom();
}

MStatus VRayThunderLoom::initialize() {
    MFnNumericAttribute nAttr; 
    MFnTypedAttribute tAttr;
    MFnStringData strData;
    
    // Create and add output attributes
    // outColor
    m_outColor = nAttr.createColor("outColor", "oc");
    CHECK_MSTATUS(nAttr.setKeyable(false));
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));
    CHECK_MSTATUS(addAttribute(m_outColor));

    // outTransparency
    m_outTransparency = nAttr.createColor("outTransparency", "ot");
    CHECK_MSTATUS(nAttr.setKeyable(false));
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));
    CHECK_MSTATUS(addAttribute(m_outTransparency));

    // outApiType
    strData.create(VRayThunderLoom::apiType);
    m_outApiType = tAttr.create("outApiType", "oat", MFnData::kString, strData.object());
    CHECK_MSTATUS(tAttr.setStorable(true));
    CHECK_MSTATUS(tAttr.setKeyable(false));
    CHECK_MSTATUS(tAttr.setHidden(true));
    CHECK_MSTATUS(addAttribute(m_outApiType));

    // outApiClassification
    strData.create(VRayThunderLoom::classification);
    m_outApiClassification = tAttr.create("outApiClassification", "oac", MFnData::kString, strData.object());
    CHECK_MSTATUS(tAttr.setStorable(true));
    CHECK_MSTATUS(tAttr.setKeyable(false));
    CHECK_MSTATUS(tAttr.setHidden(true));
    CHECK_MSTATUS(addAttribute(m_outApiClassification));

    // Create input attributes
    m_filepath = tAttr.create("filepath", "fp", MFnData::kString, strData.create(""));
    //CHECK_MSTATUS(tAttr.usedAsFilename(true));
    CHECK_MSTATUS(tAttr.setKeyable(false));
    CHECK_MSTATUS(tAttr.setStorable(true));
    CHECK_MSTATUS(tAttr.setReadable(true));
    CHECK_MSTATUS(tAttr.setWritable(true));
    CHECK_MSTATUS(addAttribute(m_filepath));
    CHECK_MSTATUS(attributeAffects(m_filepath, m_outColor));
    
    m_uscale = nAttr.create("uscale", "usc", MFnNumericData::kFloat);
    CHECK_MSTATUS(nAttr.setDefault(1.f));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(true));
    CHECK_MSTATUS(addAttribute(m_uscale));
    CHECK_MSTATUS(attributeAffects(m_uscale, m_outColor));
    
    m_vscale = nAttr.create("vscale", "vsc", MFnNumericData::kFloat);
    CHECK_MSTATUS(nAttr.setDefault(1.f));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(true));
    CHECK_MSTATUS(addAttribute(m_vscale));
    CHECK_MSTATUS(attributeAffects(m_vscale, m_outColor));

    m_uvrotation = nAttr.create("uvrotation", "uvr", MFnNumericData::kFloat);
    CHECK_MSTATUS(nAttr.setDefault(0.f));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(true));
    CHECK_MSTATUS(addAttribute(m_uvrotation));
    CHECK_MSTATUS(attributeAffects(m_uvrotation, m_outColor));
   

// Float array values
#define TL_MAYA_SET_PARAM(name, maya_name, maya_short_name, default_val) \
    name = nAttr.create(#maya_name, #maya_short_name, MFnNumericData::kFloat); \
    CHECK_MSTATUS(nAttr.setDefault(default_val)); \
    CHECK_MSTATUS(nAttr.setMin(0.0f)); \
    CHECK_MSTATUS(nAttr.setMax(1.0f)); \
    CHECK_MSTATUS(nAttr.setKeyable(true)); \
    CHECK_MSTATUS(nAttr.setStorable(true)); \
    CHECK_MSTATUS(nAttr.setReadable(true)); \
    CHECK_MSTATUS(nAttr.setWritable(true)); \
    CHECK_MSTATUS(nAttr.setArray(true)); \
    CHECK_MSTATUS(addAttribute(name)); \
    CHECK_MSTATUS(attributeAffects(name, m_outColor));
    
#define TL_MAYA_SET_BOOL_PARAM(name, maya_name, maya_short_name) \
    name = nAttr.create(#maya_name, #maya_short_name, MFnNumericData::kFloat); \
    CHECK_MSTATUS(nAttr.setDefault(0.0f));  \
    CHECK_MSTATUS(nAttr.setStorable(true)); \
    CHECK_MSTATUS(nAttr.setReadable(true)); \
    CHECK_MSTATUS(nAttr.setWritable(true)); \
    CHECK_MSTATUS(nAttr.setArray(true));    \
    CHECK_MSTATUS(addAttribute(name)); \
    CHECK_MSTATUS(attributeAffects(name, m_outColor));

#define TL_MAYA_FLOAT_PARAMS\
        TL_MAYA_SET_PARAM(m_bend, bend, bnd, 0.5f)\
        TL_MAYA_SET_BOOL_PARAM(m_bend_on, bendOn, bndo)\
        TL_MAYA_SET_PARAM(m_yarnsize, yarnsize, siz, 0.5f) \
        TL_MAYA_SET_BOOL_PARAM(m_yarnsize_on, yarnsizeOn, sizo)\
        TL_MAYA_SET_PARAM(m_twist, twist, psi, 0.5f)\
        TL_MAYA_SET_BOOL_PARAM(m_twist_on, twistOn, psio)\
        TL_MAYA_SET_BOOL_PARAM(m_alpha_on, alphaOn, alphao)\
        TL_MAYA_SET_BOOL_PARAM(m_beta_on, betaOn, betao)\
        TL_MAYA_SET_BOOL_PARAM(m_specular_color_on, specularColorOn, sclo)\
        TL_MAYA_SET_PARAM(m_specular_color_amount, specularColorAmount, smul, 1.f)\
        TL_MAYA_SET_BOOL_PARAM(m_specular_color_amount_on, specularColorAmountOn, smulo)\
        TL_MAYA_SET_PARAM(m_specular_noise, specularNoise, noi, 0.4f)\
        TL_MAYA_SET_BOOL_PARAM(m_specular_noise_on, specularNoiseOn, noio)\
        TL_MAYA_SET_PARAM(m_highlight_width, highlightWidth, hlw, 0.4f)\
        TL_MAYA_SET_BOOL_PARAM(m_highlight_width_on, highlightWidthOn, hlwo) \
        TL_MAYA_SET_BOOL_PARAM(m_diffuse_color_on, diffuseColorOn, dclo) \
        TL_MAYA_SET_PARAM(m_diffuse_color_amount, diffuseColorAmount, mul, 1.f)\
        TL_MAYA_SET_BOOL_PARAM(m_diffuse_color_amount_on, diffuseColorAmountOn, mulo)\

TL_MAYA_FLOAT_PARAMS

#undef TL_MAYA_FLOAT_PARAMS
#undef TL_MAYA_SET_PARAM
    
    m_diffuse_color = nAttr.createColor("diffuseColor", "dcl");
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(true));
    CHECK_MSTATUS(nAttr.setDefault(0.f, 0.3f, 0.f));
    CHECK_MSTATUS(nAttr.setArray(true));
    CHECK_MSTATUS(addAttribute(m_diffuse_color));
    CHECK_MSTATUS(attributeAffects(m_diffuse_color, m_outColor));
    
    m_specular_color = nAttr.createColor("specularColor", "scl");
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(true));
    CHECK_MSTATUS(nAttr.setDefault(0.4f, 0.4f, 0.4f));
    CHECK_MSTATUS(nAttr.setArray(true));
    CHECK_MSTATUS(addAttribute(m_specular_color));
    CHECK_MSTATUS(attributeAffects(m_specular_color, m_outColor));
    
    m_alpha = nAttr.create("alpha", "alpha", MFnNumericData::kFloat);
    CHECK_MSTATUS(nAttr.setMin(0.0f));
    CHECK_MSTATUS(nAttr.setMax(2.0f));
    CHECK_MSTATUS(nAttr.setDefault(0.05f));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(true));
    CHECK_MSTATUS(nAttr.setArray(true));
    CHECK_MSTATUS(addAttribute(m_alpha));
    CHECK_MSTATUS(attributeAffects(m_alpha, m_outColor));
    
    m_beta = nAttr.create("beta", "beta", MFnNumericData::kFloat);
    CHECK_MSTATUS(nAttr.setMin(0.0f));
    CHECK_MSTATUS(nAttr.setMax(6.0f));
    CHECK_MSTATUS(nAttr.setDefault(4.f));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(true));
    CHECK_MSTATUS(nAttr.setArray(true));
    CHECK_MSTATUS(addAttribute(m_beta));
    CHECK_MSTATUS(attributeAffects(m_beta, m_outColor));
    
    return MS::kSuccess;
}

// V-Ray does not call this method during rendering; it is here
// only to provide a level of compatibility with Maya.
MStatus VRayThunderLoom::compute(const MPlug &plug, MDataBlock &block) {
    if (plug == m_outColor || plug.parent() == m_outColor) {
        MDataHandle outColorHandle = block.outputValue(m_outColor);
        MFloatVector &outColor = outColorHandle.asFloatVector();
        outColor = block.inputValue(m_diffuse_color).asFloatVector();
        outColorHandle.setClean();
        return MS::kSuccess;
    }
    else if (plug == m_outTransparency || plug.parent() == m_outTransparency) {
        MDataHandle outTransparencyHandle = block.outputValue(m_outTransparency);
        MFloatVector& outTransparency = outTransparencyHandle.asFloatVector();
        outTransparency = MFloatVector(0.0f, 0.0f, 0.0f); // MFloatVector(1.0f, 1.0f, 1.0f) - block.inputValue(m_opacityMap).asFloatVector();
        outTransparencyHandle.setClean();
        return MS::kSuccess;
    }
    else {
        return MS::kUnknownParameter;
    }
}







#define TL_PARAMS\
        TL_PARAM_FLOAT(umax)\
		TL_PARAM_BOOL(umax_enabled)\
        TL_PARAM_FLOAT(yarnsize)\
		TL_PARAM_BOOL(yarnsize_enabled)\
        TL_PARAM_FLOAT(psi)\
        TL_PARAM_BOOL(psi_enabled)\
        TL_PARAM_FLOAT(alpha)\
        TL_PARAM_BOOL(alpha_enabled)\
        TL_PARAM_FLOAT(beta)\
        TL_PARAM_BOOL(beta_enabled)\
        TL_PARAM_FLOAT(specular_color.r)\
        TL_PARAM_FLOAT(specular_color.g)\
        TL_PARAM_FLOAT(specular_color.b)\
        TL_PARAM_BOOL(specular_color_enabled)\
        TL_PARAM_FLOAT(specular_amount)\
        TL_PARAM_BOOL(specular_amount_enabled)\
        TL_PARAM_FLOAT(specular_noise)\
        TL_PARAM_BOOL(specular_noise_enabled)\
        TL_PARAM_FLOAT(delta_x)\
        TL_PARAM_BOOL(delta_x_enabled)\
        TL_PARAM_FLOAT(color.r)\
        TL_PARAM_FLOAT(color.g)\
        TL_PARAM_FLOAT(color.b)\
        TL_PARAM_BOOL(color_enabled)\
        TL_PARAM_FLOAT(color_amount)\
        TL_PARAM_BOOL(color_amount_enabled)\

// ThunderLoomCommand
MStatus ThunderLoomCommand::doIt( const MArgList& args ) {
    MStatus stat;
    const char * filepath;
    //cout << "Starting doIt. \n";
    if (args.length() == 2) {
        //cout << "arg length == 2 \n";
        if (MString("-fileName") == args.asString(0, &stat) && MS::kSuccess == stat) {
            //cout << "arg is fileName \n";
            MString tmpstr = args.asString(1, &stat);
            if (MS::kSuccess == stat) {
                filepath = tmpstr.asChar();
                
                // Start to look for file
                const char *error;
                tlWeaveParameters *tl_wparams = tl_weave_pattern_from_file(filepath, &error);
                if (!tl_wparams) {
                    MPxCommand::clearResult();
                    MPxCommand::appendToResult(0.f);
                    MPxCommand::appendToResult(0.f);
					//stat = MStatus::kFailure;
                    return stat;
                }
                MPxCommand::clearResult();
                int num_yarn_types = tl_wparams->num_yarn_types;

                MPxCommand::appendToResult((float)num_yarn_types);


                // The order of the appended results has to match 
				// how they are parsed in the MEL script.
                for (int i=0; i<num_yarn_types; i++) {
                    tlYarnType* yarn_type = &tl_wparams->yarn_types[i];

#define TL_PARAM_FLOAT(name)\
	MPxCommand::appendToResult(yarn_type->name);

#define TL_PARAM_BOOL(name)\
	MPxCommand::appendToResult((float)(yarn_type->name));

TL_PARAMS

#undef TL_PARAM_FLOAT
#undef TL_PARAM_BOOL
                
                }
            }
        }
    } else {
        MString returnstr = MString("no");
        MPxCommand::clearResult();
        MPxCommand::setResult(returnstr);
    
    }
    
    return MS::kSuccess;
}

// ThunderLoomWriteCommand
MStatus ThunderLoomWriteCommand::doIt(const MArgList& args) {
	MStatus stat;
	if (args.length() > 0) {
		int save_flag = 0;
		//First argument is file name.
		const char * filepath;
		{
			MString tmpstr = args.asString(0, &stat);
			if (stat != MS::kSuccess) { return stat; };
			filepath = tmpstr.asChar();
		}

		// Start to look for file
		const char *error;
		tlWeaveParameters *tl_wparams = tl_weave_pattern_from_file(filepath, &error);
		if (!tl_wparams) {
			MPxCommand::clearResult();
			stat = MStatus::kFailure;
			return stat;
		}
		int num_yarn_types = tl_wparams->num_yarn_types;
		MPxCommand::clearResult();

		unsigned int arg = 1;
		MDoubleArray param_values = args.asDoubleArray(arg, &stat);

		// Update tl_wparams with values, same order as in ThunderLoomCommand.
		int i = 0; // Argument index
		for (int yrn_i = 0; yrn_i < num_yarn_types; yrn_i++) {
			tlYarnType* yarn_type = &tl_wparams->yarn_types[yrn_i];

			if (param_values.length() - i < 15) {
				// Stop the loop  if there aren't enough parameters 
				// to fill the 15 parameters needed for a yarn type.
				break;
			}
			save_flag = 1;

#define TL_PARAM_FLOAT(name)\
	yarn_type->name = (float)param_values[i++];

#define TL_PARAM_BOOL(name)\
	yarn_type->name = (param_values[i++] != 0.0);

TL_PARAMS

#undef TL_PARAM_FLOAT
#undef TL_PARAM_BOOL

		}

		// Save file with new params.
		if (save_flag) {
			long len = 0;
			unsigned char *data = tl_pattern_to_ptn_file(tl_wparams, &len);
			FILE *fp = fopen(filepath, "wb");
			fwrite(data, len, 1, fp);
			fclose(fp);
			free(data);
		}
		return MS::kSuccess;
	}
	return MS::kFailure;
}
#undef TL_PARAMS
