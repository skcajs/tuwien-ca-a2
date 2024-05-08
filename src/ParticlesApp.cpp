#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "CamControl.h"
#include "Particles.h"
#include "Cloth.h"
#include "cinder/params/Params.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class ParticlesApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
	void resize() override;


private:
	CameraPersp mCam;
	ParticleManager* pm;
	ClothSimulator* cs;
	params::InterfaceGlRef interfaceRef;

	float mAvgFps = 0;
	gl::BatchRef mSkyBoxBatch;
	gl::TextureCubeMapRef	mCubeMap;

	float ffRadius = 1.5f;
	vec3 ffPosition = vec3(0,0,0);
	vec3 ffForce = vec3(0, 10, 0);
	float ffPower = 3.f;
	vec3 cPosition = vec3(-2, 0, 0);
	vec3 cSize = vec3(2, 2, 2);
	bool drawMode = true;
};

void ParticlesApp::setup()
{
	pm = new ParticleManager(&mCam);
	cs = new ClothSimulator(&mCam);

	interfaceRef = params::InterfaceGl::create(getWindow(), "Particles Animation Exercise", toPixels(ivec2(225, 400)));
	interfaceRef->addParam("FPS: ", &mAvgFps);
	interfaceRef->addSeparator();
	interfaceRef->addButton("Switch Draw Mode", std::function<void()>([&] {drawMode = !drawMode; pm->setForceFieldVisibility(drawMode); }));
	interfaceRef->addParam("Wind on/off", &cs->wind);
	interfaceRef->addSeparator();
	interfaceRef->addText("Settings for new ForceFields");
	interfaceRef->addParam("Position", &ffPosition);
	interfaceRef->addParam("Radius", &ffRadius);
	interfaceRef->addParam("Force", &ffForce);
	interfaceRef->addParam("Strength", &ffPower);
	interfaceRef->addButton("New Directional Forcefield", std::bind(&ParticleManager::addDirectionalForceField, pm, ref(ffPosition), ref(ffRadius), ref(ffForce)));
	interfaceRef->addButton("New Expansion Forcefield", std::bind(&ParticleManager::addExpansionForceField, pm, ref(ffPosition), ref(ffRadius), ref(ffPower)));
	interfaceRef->addButton("New Contraction Forcefield", std::bind(&ParticleManager::addContractionForceField, pm, ref(ffPosition), ref(ffRadius), ref(ffPower)));
	interfaceRef->addButton("Remove selected Forcefield", std::bind(&ParticleManager::deleteForceField, pm));
	interfaceRef->addSeparator();
	interfaceRef->addText("Settings for new Cuboids");
	interfaceRef->addParam("Cuboid Position", &cPosition);
	interfaceRef->addParam("Cuboid Size", &cSize);
	interfaceRef->addButton("New Cuboid Obstacle", std::bind(&ParticleManager::addCuboidObstacle, pm, ref(cPosition), ref(cSize)));
	interfaceRef->addSeparator();
	interfaceRef->addText("Settings for Particles");
	interfaceRef->addParam("Bounciness", &pm->mBounciness).step(0.001f).min(0.05f).max(2.0f);
	interfaceRef->addParam("Drag coefficient", &pm->mDragCoefficient).step(0.001f).min(0.0f).max(10.0f);

	CamControl::SetCam(&mCam);
	mCam.setEyePoint(vec3(0, 0, -10));
	mCam.lookAt(vec3(0, 0, 0));
	
	//Setting up the Skybox. Feel free to change the background
	auto skyBoxGlsl = gl::GlslProg::create(loadAsset("sky_box.vert"), loadAsset("sky_box.frag"));
	mSkyBoxBatch = gl::Batch::create(geom::Cube(), skyBoxGlsl);
	mSkyBoxBatch->getGlslProg()->uniform("uCubeMapTex", 0);
	ImageSourceRef cMapImgs[6] = { loadImage(loadAsset("cubemap/posx.jpg")),loadImage(loadAsset("cubemap/negx.jpg")),loadImage(loadAsset("cubemap/posy.jpg")),
		loadImage(loadAsset("cubemap/negy.jpg")),loadImage(loadAsset("cubemap/posz.jpg")),loadImage(loadAsset("cubemap/negz.jpg")), };
	mCubeMap = gl::TextureCubeMap::create(cMapImgs, gl::TextureCubeMap::Format().mipmap());
}

void ParticlesApp::mouseDown( MouseEvent event )
{
}

void ParticlesApp::update()
{
	mAvgFps = getAverageFps();
}

void ParticlesApp::draw()
{
	gl::setMatrices(mCam);
	gl::clear( Color( 0, 0, 0 ) ); 
	gl::pushMatrices();
	mCubeMap->bind();
	gl::scale(500.0, 500.0, 500.0);
	mSkyBoxBatch->draw();
	gl::popMatrices();
	if(drawMode)
		pm->draw();
	else
		cs->draw();
	interfaceRef->draw();
}

void ParticlesApp::resize()
{
	mCam.setAspectRatio(getWindowAspectRatio());
}

CINDER_APP( ParticlesApp, RendererGl(),
	[&](App::Settings *settings) {
	settings->setWindowSize(1280, 720);
})
