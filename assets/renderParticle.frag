#version 150
#extension all : warn

out vec4 FragColor;

void main() {
	FragColor = vec4(0,1,1,1);

	vec2 pp = (gl_PointCoord*2)-vec2(1,1); //range 0..1 to -1..0..1
	if(length(pp) > 1) discard;
	 
}