#include "Cloth.h"

ClothSimulator::ClothSimulator(CameraPersp* cam)
{
	mCam = cam;
	setupBuffers();
	setupGlsl();
	mIterationsPerFrame = 20;
	getWindow()->getApp()->getSignalUpdate().connect(std::bind(&ClothSimulator::update, this));
}

void ClothSimulator::draw()
{

	// Notice that this vao holds the buffers we've just
	// written to with Transform Feedback. It will show
	// the most recent positions
	gl::ScopedVao scopeVao(mVaos[mIterationIndex & 1]);
	gl::ScopedGlslProg scopeGlsl(mRenderGlsl);
	gl::setMatrices(*mCam);
	gl::setDefaultShaderVars();

	gl::pointSize(4.0f);
	gl::drawArrays(GL_POINTS, 0, POINTS_TOTAL);

	gl::ScopedBuffer scopeBuffer(mLineIndices);
	gl::drawElements(GL_LINES, CONNECTIONS_TOTAL * 2, GL_UNSIGNED_INT, nullptr);
}

void ClothSimulator::setupBuffers() 
{
	int i, j;

	std::array<vec4, POINTS_TOTAL> positions;
	std::array<vec3, POINTS_TOTAL> velocities;
	std::array<ivec4, POINTS_TOTAL> connections;

	int n = 0;
	for (j = 0; j < POINTS_Y; j++) {
		float fj = (float)j / (float)POINTS_Y;
		for (i = 0; i < POINTS_X; i++) {
			float fi = (float)i / (float)POINTS_X;

			// create our initial positions, Basically a plane
			positions[n] = vec4((fi - 0.5f) * (float)POINTS_X * 0.2f,
				(fj - 0.5f) * (float)POINTS_Y *0.2f ,
				0.6f * sinf(fi) * cosf(fj),
				1.0f);
			// zero out velocities
			velocities[n] = vec3(0.0f);
			// to create connections we'll use an integer buffer.
			// The value -1 refers to the fact that there's no
			// connection. This helps for the edge cases of the plane
			// and also the top of the cloth which we want to be static.
			connections[n] = ivec4(-1);

			// check the edge cases and initialize the connections to be
			// basically, above, below, left, and right of this point
			if (i != (POINTS_X - 1)) {
				if (i != 0) connections[n][0] = n - 1;					// left
				if (j != 0) connections[n][1] = n - POINTS_X;				// above
				if (i != (POINTS_X - 1)) connections[n][2] = n + 1;			// right
				if (j != (POINTS_Y - 1)) connections[n][3] = n + POINTS_X;	// below
			}
			n++;
		}
	}

	for (i = 0; i < 2; i++) {
		mVaos[i] = gl::Vao::create();
		gl::ScopedVao scopeVao(mVaos[i]);
		{
			// buffer the positions
			mPositions[i] = gl::Vbo::create(GL_ARRAY_BUFFER, positions.size() * sizeof(vec4), positions.data(), GL_STATIC_DRAW);
			{
				// bind and explain the vbo to your vao so that it knows how to distribute vertices to your shaders.
				gl::ScopedBuffer sccopeBuffer(mPositions[i]);
				gl::vertexAttribPointer(POSITION_INDEX, 4, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)0);
				gl::enableVertexAttribArray(POSITION_INDEX);
			}
			// buffer the velocities
			mVelocities[i] = gl::Vbo::create(GL_ARRAY_BUFFER, velocities.size() * sizeof(vec3), velocities.data(), GL_STATIC_DRAW);
			{
				// bind and explain the vbo to your vao so that it knows how to distribute vertices to your shaders.
				gl::ScopedBuffer scopeBuffer(mVelocities[i]);
				gl::vertexAttribPointer(VELOCITY_INDEX, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)0);
				gl::enableVertexAttribArray(VELOCITY_INDEX);
			}
			// buffer the connections
			mConnections[i] = gl::Vbo::create(GL_ARRAY_BUFFER, connections.size() * sizeof(ivec4), connections.data(), GL_STATIC_DRAW);
			{
				// bind and explain the vbo to your vao so that it knows how to distribute vertices to your shaders.
				gl::ScopedBuffer scopeBuffer(mConnections[i]);
				gl::vertexAttribIPointer(CONNECTION_INDEX, 4, GL_INT, 0, (const GLvoid*)0);
				gl::enableVertexAttribArray(CONNECTION_INDEX);
			}
		}
	}
	// create your two BufferTextures that correspond to your position buffers.
	mPositionBufTexs[0] = gl::BufferTexture::create(mPositions[0], GL_RGBA32F);
	mPositionBufTexs[1] = gl::BufferTexture::create(mPositions[1], GL_RGBA32F);

	int lines = (POINTS_X - 1) * POINTS_Y + (POINTS_Y - 1) * POINTS_X;
	// create the indices to draw links between the cloth points
	mLineIndices = gl::Vbo::create(GL_ELEMENT_ARRAY_BUFFER, lines * 2 * sizeof(int), nullptr, GL_STATIC_DRAW);

	auto e = (int *)mLineIndices->mapReplace();
	for (j = 0; j < POINTS_Y; j++) {
		for (i = 0; i < POINTS_X - 1; i++) {
			*e++ = i + j * POINTS_X;
			*e++ = 1 + i + j * POINTS_X;
		}
	}

	for (i = 0; i < POINTS_X; i++) {
		for (j = 0; j < POINTS_Y - 1; j++) {
			*e++ = i + j * POINTS_X;
			*e++ = POINTS_X + i + j * POINTS_X;
		}
	}
	mLineIndices->unmap();

}

void ClothSimulator::setupGlsl()
{
	// These are the names of our out going vertices. GlslProg needs to
	// know which attributes should be captured by Transform FeedBack.
	std::vector<std::string> feedbackVaryings({
		"tf_position_mass",
		"tf_velocity"
	});

	gl::GlslProg::Format updateFormat;
	updateFormat.vertex(loadAsset("update.vert"))
		// Because we have separate buffers with which
		// to capture attributes, we're using GL_SEPERATE_ATTRIBS
		.feedbackFormat(GL_SEPARATE_ATTRIBS)
		// We also send the names of the attributes to capture
		.feedbackVaryings(feedbackVaryings);

	mUpdateGlsl = gl::GlslProg::create(updateFormat);

	gl::GlslProg::Format renderFormat;
	renderFormat.vertex(loadAsset("render.vert"))
		.fragment(loadAsset("render.frag"));

	mRenderGlsl = gl::GlslProg::create(renderFormat);
}

void ClothSimulator::update()
{
	if (!mUpdate) return;

	gl::ScopedGlslProg	scopeGlsl(mUpdateGlsl);
	gl::ScopedState		scopeState(GL_RASTERIZER_DISCARD, true);
	
	for (auto i = mIterationsPerFrame; i != 0; --i) {
		// Bind the vao that has the original vbo attached,
		// these buffers will be used to read from.
		gl::ScopedVao scopedVao(mVaos[mIterationIndex & 1]);
		// Bind the BufferTexture, which contains the positions
		// of the first vbo. We'll cycle through the neighbors
		// using the connection buffer so that we can derive our
		// next position and velocity to write to Transform Feedback
		gl::ScopedTextureBind scopeTex(mPositionBufTexs[mIterationIndex & 1]->getTarget(), mPositionBufTexs[mIterationIndex & 1]->getId());

		mUpdateGlsl->uniform("wind", wind);
		// We iterate our index so that we'll be using the
		// opposing buffers to capture the data
		mIterationIndex++;
		
		// Now bind our opposing buffers to the correct index
		// so that we can capture the values coming from the shader
		gl::bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, POSITION_INDEX, mPositions[mIterationIndex & 1]);
		gl::bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, VELOCITY_INDEX, mVelocities[mIterationIndex & 1]);
		gl::setDefaultShaderVars();
		// Begin Transform feedback with the correct primitive,
		// In this case, we want GL_POINTS, because each vertex
		// exists by itself
		gl::beginTransformFeedback(GL_POINTS);
		// Now we issue our draw command which puts all of the
		// setup in motion and processes all the vertices
		gl::drawArrays(GL_POINTS, 0, POINTS_TOTAL);
		// After that we issue an endTransformFeedback command
		// to tell OpenGL that we're finished capturing vertices
		gl::endTransformFeedback();
		
	}
}
