#include "Particles.h"
#include "cinder/Rand.h"
#include <algorithm>

bool ForceField::selectionLock = false;

ParticleManager::ParticleManager(CameraPersp* cam)
{
	mCam = cam;
	loadShaders();
	loadBuffers();
	getWindow()->getApp()->getSignalUpdate().connect(std::bind(&ParticleManager::updateUniforms, this));
	getWindow()->getApp()->getSignalUpdate().connect(std::bind(&ParticleManager::updateParticles, this));
}

void ParticleManager::updateParticles()
{
	// This equation just reliably swaps all concerned buffers
	mActiveBuffer = 1 - mActiveBuffer;

	gl::ScopedGlslProg	glslScope(mPUpdateProgRef);
	// We use this vao for input to the Glsl, while using the opposite
	// for the TransformFeedbackObj.
	gl::ScopedVao		vaoScope(mPVao[mActiveBuffer]);
	// Because we're not using a fragment shader, we need to
	// stop the rasterizer. This will make sure that OpenGL won't
	// move to the rasterization stage.
	gl::ScopedState		stateScope(GL_RASTERIZER_DISCARD, true);

	mPUpdateProgRef->uniform("Time", getElapsedFrames() / 60.0f);

	// Opposite TransformFeedbackObj to catch the calculated values
	// In the opposite buffer
	mPFeedback[1 - mActiveBuffer]->bind();


	gl::bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, mPPositions[1 - mActiveBuffer]);
	gl::bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, mPVelocities[1 - mActiveBuffer]);
	gl::bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, mPStartTimes[1 - mActiveBuffer]);
	// We begin Transform Feedback, using the same primitive that
	// we're "drawing". Using points for the particle system.
	gl::beginTransformFeedback(GL_POINTS);
	gl::drawArrays(GL_POINTS, 0, mNumParticles);
	gl::endTransformFeedback();
}

void ParticleManager::loadBuffers()
{
	Rand rand;
	std::vector<vec3> positions(mNumParticles, vec3(0.0f));
	for (auto positionsIt = positions.begin(); positionsIt != positions.end(); ++positionsIt) {
		// Creating a starting velocity
		*positionsIt = vec3(0, 0, 0) +rand.randVec3();
	}
	mPInitPosition = ci::gl::Vbo::create(GL_ARRAY_BUFFER, positions.size() * sizeof(vec3), positions.data(), GL_STATIC_DRAW);

	mPPositions[0] = ci::gl::Vbo::create(GL_ARRAY_BUFFER, positions.size() * sizeof(vec3), positions.data(), GL_STATIC_DRAW);
	mPPositions[1] = ci::gl::Vbo::create(GL_ARRAY_BUFFER, positions.size() * sizeof(vec3), nullptr, GL_STATIC_DRAW);
	std::vector<vec3> normals = std::move(positions);
	for (auto normalIt = normals.begin(); normalIt != normals.end(); ++normalIt) {
		// Creating a starting velocity
		*normalIt = vec3(-3, 2, 0);
	}

	// Create the Velocity Buffer using the newly buffered velocities
	mPVelocities[0] = ci::gl::Vbo::create(GL_ARRAY_BUFFER, normals.size() * sizeof(vec3), normals.data(), GL_STATIC_DRAW);
	// Create another Velocity Buffer that is null, for ping-ponging
	mPVelocities[1] = ci::gl::Vbo::create(GL_ARRAY_BUFFER, normals.size() * sizeof(vec3), nullptr, GL_STATIC_DRAW);
	// Create an initial velocity buffer, so that you can reset a particle's velocity after it's dead
	mPInitVelocity = ci::gl::Vbo::create(GL_ARRAY_BUFFER, normals.size() * sizeof(vec3), normals.data(), GL_STATIC_DRAW);

	// Create time data for the initialization of the particles
	vector<GLfloat> timeData;
	float time = 0.0f;
	float rate = 0.001f;
	for (int i = 0; i < mNumParticles; i++) {
		timeData.push_back(time);
		time += rate;
	}

	// Create the StartTime Buffer, so that we can reset the particle after it's dead
	mPStartTimes[0] = ci::gl::Vbo::create(GL_ARRAY_BUFFER, timeData.size() * sizeof(float), timeData.data(), GL_DYNAMIC_COPY);
	// Create the StartTime ping-pong buffer
	mPStartTimes[1] = ci::gl::Vbo::create(GL_ARRAY_BUFFER, mNumParticles * sizeof(float), nullptr, GL_DYNAMIC_COPY);

	for (int i = 0; i < 2; i++) {
		// Initialize the Vao's holding the info for each buffer
		mPVao[i] = ci::gl::Vao::create();

		// Bind the vao to capture index data for the glsl
		mPVao[i]->bind();
		mPPositions[i]->bind();
		ci::gl::vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		ci::gl::enableVertexAttribArray(0);

		mPVelocities[i]->bind();
		ci::gl::vertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		ci::gl::enableVertexAttribArray(1);

		mPStartTimes[i]->bind();
		ci::gl::vertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, 0);
		ci::gl::enableVertexAttribArray(2);

		mPInitVelocity->bind();
		ci::gl::vertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
		ci::gl::enableVertexAttribArray(3);

		mPInitPosition->bind();
		ci::gl::vertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, 0);
		ci::gl::enableVertexAttribArray(4);


		// Create a TransformFeedbackObj, which is similar to Vao
		// It's used to capture the output of a glsl and uses the
		// index of the feedback's varying variable names.
		mPFeedback[i] = gl::TransformFeedbackObj::create();

		// Bind the TransformFeedbackObj and bind each corresponding buffer
		// to it's index.
		mPFeedback[i]->bind();
		gl::bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, mPPositions[i]);
		gl::bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, mPVelocities[i]);
		gl::bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, mPStartTimes[i]);
		mPFeedback[i]->unbind();
	}
}

void ParticleManager::draw()
{
	gl::ScopedVao			vaoScope(mPVao[1 - mActiveBuffer]);
	gl::ScopedGlslProg		glslScope(mPRenderProgRef);
	gl::ScopedState			stateScope(GL_PROGRAM_POINT_SIZE, true);
	gl::ScopedBlend			blendScope(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	mPRenderProgRef->bind();
	gl::setDefaultShaderVars();
	gl::drawArrays(GL_POINTS, 0, mNumParticles);
}

void ParticleManager::addDirectionalForceField(vec3 pos, float radius, vec3 force)
{
	forceFields.push_back(make_shared<DirectionForceField>(pos, radius, force, mCam));
	forceFields.back()->connectSignals();
}

void ParticleManager::addExpansionForceField(vec3 pos, float radius, float force)
{
	forceFields.push_back(make_shared<ExpansionForceField>(pos, radius, force,mCam));
	forceFields.back()->connectSignals();
}

void ParticleManager::addContractionForceField(vec3 pos, float radius, float force)
{
	forceFields.push_back(make_shared<ContractionForceField>(pos, radius, force, mCam));
	forceFields.back()->connectSignals();
}

void ParticleManager::addCuboidObstacle(vec3 pos, vec3 size)
{
	forceFields.push_back(make_shared<CuboidObstacle>(pos, size, mCam));
	forceFields.back()->connectSignals();
}

void ParticleManager::deleteForceField()
{
	{
		auto element = find_if(forceFields.begin(), forceFields.end(), [](shared_ptr<ForceField> ff) {
			return ff->isSelected();
		});
		if(element != forceFields.end())forceFields.erase(element);
	}

}

void ParticleManager::setForceFieldVisibility(bool visible)
{
	if(visible)
		for_each(forceFields.begin(), forceFields.end(), [&](shared_ptr<ForceField> ff) {
			ff->connectSignals();
		});
	else
		for_each(forceFields.begin(), forceFields.end(), [&](shared_ptr<ForceField> ff) {
		ff->connectionList.clear();
	});
}



void ParticleManager::loadShaders()
{
	std::vector<std::string> varyings(3);
	varyings[0] = "Position";
	varyings[1] = "Velocity";
	varyings[2] = "StartTime";

	ci::gl::GlslProg::Format updateProgFormat;

	updateProgFormat.vertex(loadAsset("updateParticles.vert"));
	updateProgFormat.feedbackFormat(GL_SEPARATE_ATTRIBS)
		.feedbackVaryings(varyings)
		.attribLocation("VertexPosition", 0)
		.attribLocation("VertexVelocity", 1)
		.attribLocation("VertexStartTime", 2)
		.attribLocation("VertexInitialVelocity", 3);
	mPUpdateProgRef = ci::gl::GlslProg::create(updateProgFormat);
	mPUpdateProgRef->uniform("H", 1.0f / 60.0f);

	ci::gl::GlslProg::Format renderProgFormat;
	renderProgFormat.vertex(loadAsset("renderParticle.vert"))
		.fragment(loadAsset("renderParticle.frag"))
		.attribLocation("VertexPosition", 0);
      //.attribLocation("VertexStartTime", 2);
	mPRenderProgRef = ci::gl::GlslProg::create(renderProgFormat);
	//mPRenderProgRef->uniform("ParticleLifetime", mParticleLifetime);
	mPRenderProgRef->uniform("ParticleBounciness", mBounciness);
	mPUpdateProgRef->uniform("ParticleBounciness", mBounciness);
	mPUpdateProgRef->uniform("DragCoefficient", mDragCoefficient);
}

void ParticleManager::updateUniforms()
{

	int directionalForceFields = 0, expansionForceFields = 0, contractionForceFields = 0;
	int cuboidObstacles = 0;
	for_each(forceFields.begin(), forceFields.end(), [&](shared_ptr<ForceField> ff) {

		switch (ff->type)
		{
		case(Directional):
		{
			auto *dff = (DirectionForceField*)(ff.get());
			string loc = "directionalForceFields[" + to_string(directionalForceFields) + "].";
			mPUpdateProgRef->uniform(loc + "position", dff->position);
			mPUpdateProgRef->uniform(loc + "radius", dff->radius);
			mPUpdateProgRef->uniform(loc + "force", dff->force);
			++directionalForceFields;
			break;
		}
		case(Expansion):
		{
			auto *eff = (ExpansionForceField*)(ff.get());
			string loc = "expansionForceFields[" + to_string(expansionForceFields) + "].";
			mPUpdateProgRef->uniform(loc + "position", eff->position);
			mPUpdateProgRef->uniform(loc + "radius", eff->radius);
			mPUpdateProgRef->uniform(loc + "force", eff->force);
			++expansionForceFields;
			break;
		}
		case(Contraction):
		{
			auto *cff = (ContractionForceField*)(ff.get());
			string loc = "contractionForceFields[" + to_string(contractionForceFields) + "].";
			mPUpdateProgRef->uniform(loc + "position", cff->position);
			mPUpdateProgRef->uniform(loc + "radius", cff->radius);
			mPUpdateProgRef->uniform(loc + "force", cff->force);
			++contractionForceFields;
			break;
		}
		case(CObstacle):
		{
			auto cob = (CuboidObstacle*)(ff.get());
			string loc = "cuboidObstacles[" + to_string(cuboidObstacles) + "].";
			mPUpdateProgRef->uniform(loc + "pos", cob->position - cob->size/2.f);
			mPUpdateProgRef->uniform(loc + "size", cob->size);
			++cuboidObstacles;
			break;
		}
		}
	});
	mPUpdateProgRef->uniform("numDirectionalForceFields", directionalForceFields);
	mPUpdateProgRef->uniform("numExpansionForceFields", expansionForceFields);
	mPUpdateProgRef->uniform("numContractionForceFields", contractionForceFields);
	mPUpdateProgRef->uniform("numCuboidObstacles", cuboidObstacles);
	mPUpdateProgRef->uniform("ParticleBounciness", mBounciness);
	mPUpdateProgRef->uniform("DragCoefficient", mDragCoefficient);
}



ForceField::ForceField(vec3 pos, CameraPersp* c)
{
	position = pos;
	cam = c;
}

bool ForceField::isSelected()
{
	return selected;
}


SphericalForceField::SphericalForceField(vec3 pos, float radius_, CameraPersp * c) : ForceField(pos, c)
{
	radius = radius_;

	gl::GlslProgRef progRef = gl::getStockShader(gl::ShaderDef().color());
	batchRef = gl::Batch::create(geom::Sphere().radius(radius), progRef);
}

SphericalForceField::~SphericalForceField()
{
	connectionList.clear();
	selectionLock = false;
}

void SphericalForceField::connectSignals()
{
	connectionList.add(getWindow()->getSignalMouseDown().connect(std::bind(&SphericalForceField::mouseDown, this, std::placeholders::_1)));
	connectionList.add(getWindow()->getSignalMouseDrag().connect(std::bind(&SphericalForceField::mouseDrag, this, std::placeholders::_1)));
	connectionList.add(getWindow()->getSignalPostDraw().connect(std::bind(&SphericalForceField::draw, this)));
}


void SphericalForceField::mouseDown(MouseEvent e)
{
	if (selectionLock && !selected) return;
	if (!e.isLeft() || !e.isLeftDown()) return; //Only react to LMB
												//Calculate the collision ray
	Ray ray = cam->generateRay(e.getPos(), getWindow()->getSize());

	selectedHandle = -1;
	//if the volume is selected, collision test against vectors
	if (selected) {
		Sphere collisionSphere = Sphere(position + vec3(1, 0, 0) + vec3(1, 0, 0)*radius, 0.2f);
		if (collisionSphere.intersects(ray))
			selectedHandle = 0;
		collisionSphere = Sphere(position + vec3(0, 1, 0) + vec3(0, 1, 0)*radius, 0.2f);
		if (collisionSphere.intersects(ray))
			selectedHandle = 1;
		collisionSphere = Sphere(position + vec3(0, 0, 1) + vec3(0, 0, 1)*radius, 0.2f);
		if (collisionSphere.intersects(ray))
			selectedHandle = 2;
	}
	//If the Volume is not currently being moved, check for intersection
	if (selectedHandle == -1) {
		Sphere collisionSphere = Sphere(position, radius);
		if (collisionSphere.intersects(ray)) {
			selected = true;
			selectionLock = true;
		}
		else {
			selected = false;
			selectionLock = false;
		}

	}
}
void SphericalForceField::mouseDrag(MouseEvent e)
{
	if (!selected || !e.isLeftDown())return;
	Ray ray = cam->generateRay(e.getPos(), getWindow()->getSize());
	if (selectedHandle == 0) {
		float dist;
		ray.calcPlaneIntersection(position, vec3(0, 0, 1), &dist);
		position.x = (ray.getOrigin() + ray.getDirection() * dist).x - 1 - radius;
	}
	if (selectedHandle == 1) {
		float dist;
		ray.calcPlaneIntersection(position, vec3(0, 0, 1), &dist);
		position.y = (ray.getOrigin() + ray.getDirection() * dist).y - 1 - radius;
	}
	if (selectedHandle == 2) {
		float dist;
		ray.calcPlaneIntersection(position, vec3(0, 1, 0), &dist);
		position.z = (ray.getOrigin() + ray.getDirection() * dist).z - 1 - radius;
	}

}
void SphericalForceField::draw()
{
	//Draw the volume
	gl::pushMatrices();
	gl::translate(position);
	gl::color(ffColor.r, ffColor.g, ffColor.b, ffColor.a);
	batchRef->draw();

	if (selected) { //Draw the handles
		gl::color(1, 0, 0, 1);
		gl::drawVector(vec3(1, 0, 0)*radius, vec3(1, 0, 0) + vec3(1, 0, 0)*radius);
		gl::color(0, 1, 0, 1);
		gl::drawVector(vec3(0, 1, 0)*radius, vec3(0, 1, 0) + vec3(0, 1, 0)*radius);
		gl::color(0, 0, 1, 1);
		gl::drawVector(vec3(0, 0, 1)*radius, vec3(0, 0, 1) + vec3(0, 0, 1)*radius);
	}
	gl::popMatrices();

}

CuboidForceField::CuboidForceField(vec3 pos, vec3 size_, CameraPersp* c) : ForceField(pos, c)
{
	position = pos;
	size = size_;
	gl::GlslProgRef progRef = gl::getStockShader(gl::ShaderDef().color());
	batchRef = gl::Batch::create(geom::Cube().size(size), progRef);
}

CuboidForceField::~CuboidForceField()
{
	connectionList.clear();
	selectionLock = false;
}

void CuboidForceField::connectSignals()
{
	connectionList.add(getWindow()->getSignalMouseDown().connect(std::bind(&CuboidForceField::mouseDown, this, std::placeholders::_1)));
	connectionList.add(getWindow()->getSignalMouseDrag().connect(std::bind(&CuboidForceField::mouseDrag, this, std::placeholders::_1)));
	connectionList.add(getWindow()->getSignalPostDraw().connect(std::bind(&CuboidForceField::draw, this)));
}

void CuboidForceField::mouseDown(MouseEvent e)
{
	if (selectionLock && !selected) return;
	if (!e.isLeft() || !e.isLeftDown()) return; //Only react to LMB
												//Calculate the collision ray
	Ray ray = cam->generateRay(e.getPos(), getWindow()->getSize());
	selectedHandle = -1;
	if (selected) {
		Sphere collisionSphere = Sphere(position + vec3(0.9f + size.x / 2, 0, 0), 0.2f);
		if (collisionSphere.intersects(ray))
			selectedHandle = 0;
		collisionSphere = Sphere(position + vec3(0, 0.9f + size.y / 2, 0), 0.2f);
		if (collisionSphere.intersects(ray))
			selectedHandle = 1;
		collisionSphere = Sphere(position + vec3(0, 0, 0.9f + size.z / 2), 0.2f);
		if (collisionSphere.intersects(ray))
			selectedHandle = 2;
	}
	if (selectedHandle == -1) {
		AxisAlignedBox box(position - size / 2.f, position + size / 2.f);
		if (box.intersects(ray)) {
			selected = true;
			selectionLock = true;
		}
		else {
			selected = false;
			selectionLock = false;
		}
	}
}

void CuboidForceField::mouseDrag(MouseEvent e)
{
	if (!selected || !e.isLeftDown())return;
	Ray ray = cam->generateRay(e.getPos(), getWindow()->getSize());
	if (selectedHandle == 0) {
		float dist;
		ray.calcPlaneIntersection(position, vec3(0, 0, 1), &dist);
		position.x = (ray.getOrigin() + ray.getDirection() * dist).x - 1 - size.x / 2;
	}
	if (selectedHandle == 1) {
		float dist;
		ray.calcPlaneIntersection(position, vec3(0, 0, 1), &dist);
		position.y = (ray.getOrigin() + ray.getDirection() * dist).y - 1 - size.y / 2;
	}
	if (selectedHandle == 2) {
		float dist;
		ray.calcPlaneIntersection(position, vec3(0, 1, 0), &dist);
		position.z = (ray.getOrigin() + ray.getDirection() * dist).z - 1 - size.z / 2;
	}
}

void CuboidForceField::draw()
{
	gl::pushMatrices();
	gl::translate(position);
	gl::color(ffColor.r, ffColor.g, ffColor.b, ffColor.a);
	batchRef->draw();
	if (selected) { //Draw the handles
		gl::color(1, 0, 0, 1);
		gl::drawVector(vec3(size.x / 2.f, 0, 0), vec3(1 + size.x / 2.f, 0, 0));
		gl::color(0, 1, 0, 1);
		gl::drawVector(vec3(0, size.y / 2.f, 0), vec3(0, 1 + size.y / 2.f, 0));
		gl::color(0, 0, 1, 1);
		gl::drawVector(vec3(0, 0, size.z / 2.f), vec3(0, 0, 1 + size.z / 2.f));
	}
	gl::popMatrices();
}


DirectionForceField::DirectionForceField(vec3 pos, float rad, vec3 force_, CameraPersp* c) : SphericalForceField(pos, rad, c)
{
	force = force_;
	type = Directional;
	ffColor = vec4(1, 0, 0, 0.1f);
}


ExpansionForceField::ExpansionForceField(vec3 pos, float radius,float force_, CameraPersp * c) : SphericalForceField(pos, radius, c)
{
	force = force_;
	type = Expansion;
	ffColor = vec4(0, 1, 0, 0.1f);
}

ContractionForceField::ContractionForceField(vec3 pos, float radius,float force_, CameraPersp * c) : SphericalForceField(pos, radius, c)
{
	force = force_;
	type = Contraction;
	ffColor = vec4(0, 0, 1, 0.1f);
}

CuboidObstacle::CuboidObstacle(vec3 pos, vec3 size, CameraPersp * c) : CuboidForceField(pos,size,c)
{
	type = CObstacle;
	ffColor = vec4(0, 1, 1, 0.1f);
}
