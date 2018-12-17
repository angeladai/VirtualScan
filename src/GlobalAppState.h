#pragma once

#include "stdafx.h"

#include <vector>
#include <string>
#include <list>

#define PRINT_CAMERAS

#define X_GLOBAL_APP_STATE_FIELDS \
	X(std::string, s_meshDirectory) \
	X(std::string, s_meshFileList) \
	X(std::string, s_outDirectory) \
	X(unsigned int, s_imageWidth) \
	X(unsigned int, s_imageHeight) \
	X(float, s_cameraFov) \
	X(bool, s_addNoiseToDepth) \
	X(float, s_depthNoiseSigma) \
	X(bool, s_filterDepthMap) \
	X(float, s_depthSigmaD) \
	X(float, s_depthSigmaR) \
	X(float, s_depthMin) \
	X(float, s_depthMax) \
	X(unsigned int, s_gridDim) \
	X(unsigned int, s_padding) \
	X(unsigned int, s_numTrajectories) \
	X(unsigned int, s_maxTrajectoryLength) \
	X(unsigned int, s_maxModels) \
	X(bool, s_generatePartial) \
	X(std::string, s_trajectoryDir) \
	X(bool, s_scanFromTrajectoryFiles) \
	X(std::string, s_cameraDir) \
	X(bool, s_generatePointClouds) \
	X(unsigned int, s_maxPointsPerFrame) \
	X(bool, s_scanWholeShape) \
	X(bool, s_transformToUnitBox) \
	X(bool, s_debugOutput) 


#ifndef VAR_NAME
#define VAR_NAME(x) #x
#endif

#define checkSizeArray(a, d)( (((sizeof a)/(sizeof a[0])) >= d))

class GlobalAppState
{
public:

#define X(type, name) type name;
	X_GLOBAL_APP_STATE_FIELDS
#undef X

		//! sets the parameter file and reads
	void readMembers(const ParameterFile& parameterFile) {
		m_ParameterFile = parameterFile;
		readMembers();
	}

	//! reads all the members from the given parameter file (could be called for reloading)
	void readMembers() {
#define X(type, name) \
	if (!m_ParameterFile.readParameter(std::string(#name), name)) {MLIB_WARNING(std::string(#name).append(" ").append("uninitialized"));	name = type();}
		X_GLOBAL_APP_STATE_FIELDS
#undef X
 

		m_bIsInitialized = true;
	}

	void print() const {
#define X(type, name) \
	std::cout << #name " = " << name << std::endl;
		X_GLOBAL_APP_STATE_FIELDS
#undef X
	}

	static GlobalAppState& getInstance() {
		static GlobalAppState s;
		return s;
	}
	static GlobalAppState& get() {
		return getInstance();
	}


	//! constructor
	GlobalAppState() {
		m_bIsInitialized = false;
	}

	//! destructor
	~GlobalAppState() {
	}

	Timer	s_Timer;

private:
	bool			m_bIsInitialized;
	ParameterFile	m_ParameterFile;
};
