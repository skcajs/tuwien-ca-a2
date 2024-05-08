#pragma once
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "Particles.h"
#include "cinder/Noncopyable.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace ci::signals;


struct Particle {
	float mass;
	vec3 velocity;
	vec3 pos;

};

enum ForceFieldType{Directional, Expansion, Contraction, CObstacle};

class ForceField{
public:
	ForceField(vec3 pos, CameraPersp* cam);
	virtual void connectSignals() =  0;
	virtual void mouseDown(MouseEvent e) = 0;
	virtual void mouseDrag(MouseEvent e) = 0;
	virtual void draw() = 0;	

	bool isSelected();

	vec3 position;
	ForceFieldType type;
	ConnectionList connectionList;

protected:
	vec4 ffColor;
	bool selected = false;
	static bool selectionLock;
	int selectedHandle = -1;
	CameraPersp* cam;
	
};

class SphericalForceField : public ForceField{
public:
	SphericalForceField(vec3 pos, float radius_, CameraPersp* c);
	~SphericalForceField();
	void connectSignals();
	void mouseDown(MouseEvent e);
	void mouseDrag(MouseEvent e);
	void draw();

	float radius;
protected:
	gl::BatchRef batchRef;
};

class CuboidForceField : public ForceField {
public:
	CuboidForceField(vec3 pos, vec3 size, CameraPersp* c);
	~CuboidForceField();
	void connectSignals();
	void mouseDown(MouseEvent e);
	void mouseDrag(MouseEvent e);
	void draw();

	vec3 size;
protected:
	gl::BatchRef batchRef;
};

class CuboidObstacle : public CuboidForceField{
public:
	CuboidObstacle(vec3 pos, vec3 size, CameraPersp* c);
};

class DirectionForceField : public SphericalForceField {
public: 
	DirectionForceField(vec3 pos,float radius, vec3 force_, CameraPersp* c);
	vec3 force;
	
};

class ExpansionForceField : public SphericalForceField {
public:
	ExpansionForceField(vec3 pos, float radius,float force, CameraPersp* c);
	float force;
};

class ContractionForceField : public SphericalForceField {
public:
	ContractionForceField(vec3 pos, float radius, float force, CameraPersp* c);
	float force;
};

class ParticleManager {
public:
	ParticleManager(CameraPersp* cam);
	void updateParticles();
	void loadBuffers();
	void draw();
	
	void addDirectionalForceField(vec3 pos, float radius, vec3 force);
	void addExpansionForceField(vec3 pos, float radius, float force);
	void addContractionForceField(vec3 pos, float radius, float force);
	void addCuboidObstacle(vec3 pos, vec3 size);
	void deleteForceField();

	void setForceFieldVisibility(bool visible);

	float mBounciness = 0.01f; /* Particle bounciness */
	float mDragCoefficient = 0.0f;
	float mParticleLifetime = 3.0f; /* Particle lifetime */
private:
	int mNumParticles = 2900;
	gl::VaoRef	mPVao[2];
	gl::TransformFeedbackObjRef mPFeedback[2];
	gl::VboRef	mPPositions[2], mPVelocities[2], mPStartTimes[2], mPInitVelocity, mPInitPosition;
	gl::GlslProgRef mPUpdateProgRef, mPRenderProgRef;
	int mActiveBuffer = 1;
	CameraPersp* mCam;

	std::list<shared_ptr<ForceField>> forceFields;

	void loadShaders();
	void updateUniforms();

};