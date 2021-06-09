#version 330 core 

out vec4 color;

uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;
uniform float MatShine;
uniform vec3 camPos;

//interpolated normal and light vector in camera space
in vec3 fragNor;
in vec3 lightDir;
//position of the vertex in camera space
in vec3 EPos;

void main()
{
	vec3 normal = normalize(fragNor);
	vec3 light = normalize(lightDir);
	// Map normal in the range [-1, 1] to color in range [0, 1];

	vec3 Ncolor = 0.5*normal + vec3(1, .3, -.2);
	color = vec4(Ncolor, 1.0);
	// color = vec4(1.0, 0.5, 0.0, 1.0);
}
