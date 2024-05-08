#pragma once
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

using namespace ci;
using namespace ci::app;
using namespace std;

const uint32_t POINTS_X = 10;
const uint32_t POINTS_Y = 10;
const uint32_t POINTS_TOTAL = (POINTS_X * POINTS_Y);
const uint32_t CONNECTIONS_TOTAL = (POINTS_X - 1) * POINTS_Y + (POINTS_Y - 1) * POINTS_X;

const uint32_t POSITION_INDEX = 0;
const uint32_t VELOCITY_INDEX = 1;
const uint32_t CONNECTION_INDEX = 2;

class ClothSimulator
{
public:
	ClothSimulator(CameraPersp* cam);
	void draw();

	bool wind = true;

private:

	void setupBuffers();
	void setupGlsl();
	void update();
	

	std::array<gl::VaoRef, 2>			mVaos;
	std::array<gl::VboRef, 2>			mPositions, mVelocities, mConnections;
	std::array<gl::BufferTextureRef, 2>	mPositionBufTexs;
	gl::VboRef							mLineIndices;
	gl::GlslProgRef						mUpdateGlsl, mRenderGlsl;

	float								mCurrentCamRotation;
	uint32_t							mIterationsPerFrame, mIterationIndex;
	bool								mDrawPoints, mDrawLines, mUpdate;
	CameraPersp*	mCam;

	ci::params::InterfaceGlRef			mParams;
};