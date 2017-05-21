// Check for 64-bit OS and define the Bits64_ macro required by Maya headers
//#include "defines.h"
//#ifdef X64
//#define Bits64_
//#endif

// Maya headers
#include <maya/MFnPlugin.h>
#include <maya/MPxLocatorNode.h>
#include <maya/MGlobal.h>

#define VERSION 0.93.0
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#include "maya_thunderloom.h"

// Maya docs: The first of these methods is called by Maya when the plug-in is
// loaded. Its purpose is to create an instance of the MFnPlugin class
// (initialized with the MObject passed to the routine) and call register
// methods in that class to inform Maya what it is capable of doing.
PLUGIN_EXPORT MStatus initializePlugin(MObject obj)
{
	MStatus stat;
	MString errStr;
	MFnPlugin plugin(obj, "https://github.com/vidarn/ThunderLoom", STR(VERSION), "Any", &stat);

	if (!stat) {
		MGlobal::displayInfo("Could not initialize MFnPlugin.\n");
		errStr = "failed to initialize plugin";
		stat.perror(errStr);
	}

	// Register the VRayPlane node
	const MString classification(VRayThunderLoom::classification);
	stat=plugin.registerNode("VRayThunderLoomMtl", VRayThunderLoom::id, VRayThunderLoom::creator, VRayThunderLoom::initialize, MPxNode::kDependNode, &classification);
	if (!stat) { errStr = "registerNode failed";  stat.perror(errStr); return stat; }
	
	stat=plugin.registerCommand("thunderLoomParseFile", ThunderLoomCommand::creator);
	if (!stat) { errStr = "registerCommand failed";  stat.perror(errStr); return stat; }

	return stat;
}

PLUGIN_EXPORT MStatus uninitializePlugin(MObject obj)
{
	MStatus stat;
	MString errStr;
	MFnPlugin plugin(obj);

	// Deregister the node
	stat = plugin.deregisterNode(VRayThunderLoom::id);
	if (!stat) { errStr = "deregisterNode failed";  stat.perror(errStr); }

	return stat;
}
// V-Ray docs: V-Ray ignores the result from the compute() method of the node
// (the corresponding V-Ray plugin is used for rendering); however you can
// implement a suitable representation of the material for the Maya software 
// renderer, if you wish.
// compute...
