#version 150 core

in vec3 VertexPosition;
in vec3 VertexVelocity;
in float VertexStartTime;
in vec3 VertexInitialVelocity;
in vec3 VertexInitialPosition;
in vec4 VertexColor;

out vec3 Position; // To Transform Feedback
out vec3 Velocity; // To Transform Feedback
out vec4 Color; // To Transform Feedback
out float StartTime; // To Transform Feedback

uniform float Time; // Time
uniform float H;	// Elapsed time between frames
uniform float ParticleLifetime = 3.0f; // Particle lifespan
uniform float ParticleBounciness = 0.01f; // Particle bounciness
uniform float DragCoefficient = 0.0f; // Drag coefficient

struct directionalForceField{
	vec3 position;
	float radius;
	vec3 force;
};

uniform int numDirectionalForceFields;
uniform directionalForceField directionalForceFields[10];

struct expansionForceField{
	vec3 position;
	float radius;
	float force;
};

uniform int numExpansionForceFields;
uniform expansionForceField expansionForceFields[10];

struct contractionForceField{
	vec3 position;
	float radius;
	float force;
};

uniform int numContractionForceFields;
uniform contractionForceField contractionForceFields[10];

struct cuboidObstacle{
	vec3 pos;      //Position are the smallest x,y,z coordinates, not the center
	vec3 size;
};

uniform int numCuboidObstacles;
uniform cuboidObstacle cuboidObstacles[10];

bool pointInSphere(vec3 pos, vec3 c, float r) {
    return pow(pos.x - c.x, 2) + pow(pos.y - c.y, 2) + pow(pos.z - c.z, 2) <= pow(r,2);
}

//todo: Task2, please read the help section!
vec3 getDirectionalForceFieldInfluence(vec3 pos){
	vec3 totalForce = vec3(0,0,0);

	for (int i = 0; i < numDirectionalForceFields; ++i) {
		directionalForceField field = directionalForceFields[i];
		if (pointInSphere(pos, field.position, field.radius)) {
			totalForce += field.force;
		}
	}

	return totalForce;
}

vec3 getExpansionForceFieldInfluence(vec3 pos){
	vec3 totalForce = vec3(0,0,0);

	for (int i = 0; i < numExpansionForceFields; ++i) {
		expansionForceField field = expansionForceFields[i];
		if (pointInSphere(pos, field.position, field.radius)) {
			totalForce += normalize(pos - field.position) * field.radius * field.force * 5.0;
		}
	}

	return totalForce;
}

vec3 getContractionForceFieldInfluence(vec3 pos){
	vec3 totalForce = vec3(0,0,0);

	for (int i = 0; i < numContractionForceFields; ++i) {
		contractionForceField field = contractionForceFields[i];
		if (pointInSphere(pos, field.position, field.radius)) {
			totalForce += normalize(field.position-pos) * field.radius * field.force * 15.0;
		}
	}

	return totalForce;
}

vec3 getDragForce(vec3 velocity) {
	return velocity * velocity * DragCoefficient * 0.005;
}

bool collisionDetected(vec3 pos) {
	bool collision = false;
	for (int i = 0; i < numCuboidObstacles; ++i) {
		cuboidObstacle obs = cuboidObstacles[i];
		vec3 obsMin = obs.pos, obsMax = obs.pos + obs.size;
		if (pos.x >= obsMin.x && pos.x <= obsMax.x &&
			pos.y >= obsMin.y && pos.y <= obsMax.y &&
			pos.z >= obsMin.z && pos.z <= obsMax.z) collision = true;
	}
	return collision;
}

vec3 getCollisionVelocityComponents(vec3 pos) {

	for (int i = 0; i < numCuboidObstacles; ++i) {
		cuboidObstacle obs = cuboidObstacles[i];
		vec3 obsMin = obs.pos, obsMax = obs.pos + obs.size;

		if (pos.x >= obsMin.x + 0.1 && pos.x <= obsMax.x - 0.1 &&
			pos.y >= obsMin.y + 0.1 && pos.y <= obsMax.y  - 0.1 &&
			pos.z >= obsMin.z + 0.1 && pos.z <= obsMax.z  - 0.1) Position = vec3(pos.x, -100, pos.z); // Out of view

		if( pos.y < obsMax.y &&
			pos.x > obsMin.x + 0.05 && pos.x < obsMax.x -0.05 &&
			pos.x > obsMin.x +0.05 && pos.x < obsMax.x -0.05) return vec3(Velocity.x * ParticleBounciness, -Velocity.y * ParticleBounciness, Velocity.z * ParticleBounciness);
	}

	return vec3(-Velocity.x * ParticleBounciness, Velocity.y, -Velocity.z * ParticleBounciness );
}

// Gravity
uniform vec3 gravity = vec3(0.0, -10.0, 0.0);

void main() {
	// Update position & velocity for next frame
	Position = VertexPosition;

	Velocity = VertexVelocity + gravity * H;
	Velocity += getDirectionalForceFieldInfluence(Position) * H;
	Velocity += getExpansionForceFieldInfluence(Position) * H;
	Velocity += getContractionForceFieldInfluence(Position) * H;
	vec3 dragForce = getDragForce(Velocity);
	Velocity += dragForce;

	if (collisionDetected(Position)) {
		Velocity = getCollisionVelocityComponents(Position);
	}
	StartTime = VertexStartTime;

	if( Time >= StartTime ) {
		
		float age = Time - StartTime;
		
		if( age > ParticleLifetime ) {
			// The particle is past it's lifetime, recycle.
			Position = VertexInitialPosition;
			Velocity = VertexInitialVelocity;
			StartTime = Time;
		}
		else {
			// The particle is alive, update.
			vec3 oldPos = Position;

			Position += Velocity * H;
		}
	}
}