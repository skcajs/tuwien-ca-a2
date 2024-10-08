// Update Vertex Shader
// OpenGL SuperBible Chapter 7
// Graham Sellers
#version 330 core
#define M_PI 3.1415926535897932384626433832795

// This input vector contains the vertex position in xyz, and the
// mass of the vertex in w
layout (location = 0) in vec4 position_mass;	// POSITION_INDEX
// This is the current velocity of the vertex
layout (location = 1) in vec3 velocity;			// VELOCITY_INDEX
// This is our connection vector
layout (location = 2) in ivec4 connection;		// CONNECTION_INDEX

// This is a TBO that will be bound to the same buffer as the
// position_mass input attribute
uniform samplerBuffer tex_position;

uniform float ciElapsedSeconds;
uniform bool wind;

// The outputs of the vertex shader are the same as the inputs
out vec4 tf_position_mass;
out vec3 tf_velocity;

// A uniform to hold the timestep. The application can update this.
uniform float t = 0.05;

// The global spring constant
uniform float k = 7.1;

// Gravity
uniform vec3 gravity = vec3(0.0, -0.01, 0.0);

// Global damping constant
uniform float c = 1.8;

// Spring resting length
uniform float rest_length = 0.2;


void main(void)
{
	// todo Task1: modify this function to implement a wind force
	// useful variables: bool wind (ui "wind on/off" will set this variable)
	// float ciElapsedSeconds  (the running duration of the program)

	vec3 p = position_mass.xyz;    // p can be our position
	
	float m = position_mass.w;     // m is the mass of our vertex
	vec3 u = velocity;             // u is the initial velocity
	vec3 F = gravity *  m - c * u;  // F is the force on the mass
	bool fixed_node = true;        // Becomes false when force is applied
	
	for( int i = 0; i < 4; i++) {
		if( connection[i] != -1 ) {
			// q is the position of the other vertex
			vec3 q = texelFetch(tex_position, connection[i]).xyz;
			vec3 d = q - p;
			float x = length(d);
			F += -k * (rest_length - x) * normalize(d);
			fixed_node = false;
		}
	}
	
	// If this is a fixed node, reset force to zero
	if( fixed_node ) {
		F = vec3(0.0);
	}

	// Accelleration due to force
	vec3 a = F / m;
	
	// Displacement
	vec3 s = u * t + 0.5 * a * t * t;
	
	// Final velocity
	vec3 v = u + a * t;
	
	// Constrain the absolute value of the displacement per step
	s = clamp(s, vec3(-25.0), vec3(25.0));
	
	// Write the outputs
	tf_position_mass = vec4(p + s, m);
	tf_velocity = v;
}
