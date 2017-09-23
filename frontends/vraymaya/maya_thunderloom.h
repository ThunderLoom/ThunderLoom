#ifndef __VRAYTHUNDERLOOM_H_
#define __VRAYTHUNDERLOOM_H_

//#include <maya/MSimple.h>
//#include <maya/MIOStream.h>
#include <maya/MPxCommand.h>
#include <maya/MArgList.h>



class VRayThunderLoom: public MPxNode {
public:
	VRayThunderLoom();
	virtual ~VRayThunderLoom();

	virtual MStatus compute(const MPlug&, MDataBlock&);

	static void* creator();
	static MStatus initialize();
	
	static MTypeId id;
	static MString apiType;
	static MString classification;
private:
	static MObject m_filepath;
	static MObject m_uscale;
	static MObject m_vscale;
	static MObject m_uvrotation;
	static MObject m_bend;
	static MObject m_bend_on;
	static MObject m_yarnsize;
	static MObject m_yarnsize_on;
	static MObject m_twist;
	static MObject m_twist_on;
	static MObject m_specular_color;
	static MObject m_specular_color_on;
    static MObject m_specular_color_amount;
 	static MObject m_specular_color_amount_on;
	static MObject m_specular_noise;
	static MObject m_specular_noise_on;
	static MObject m_highlight_width;
	static MObject m_highlight_width_on;
	static MObject m_diffuse_color;
	static MObject m_diffuse_color_on;
    static MObject m_diffuse_color_amount;
 	static MObject m_diffuse_color_amount_on;

	static MObject m_outColor;
	static MObject m_outTransparency;
	static MObject m_outApiType;
	static MObject m_outApiClassification;
};

class ThunderLoomCommand: public MPxCommand {
    public:
        MStatus doIt( const MArgList& args);
        static void* creator();
};


#endif
