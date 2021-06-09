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
in vec3 lightDir2;
in vec3 lightDir3;
//position of the vertex in camera space
in vec3 EPos;

void main()
{
	//you will need to work with these for lighting
	vec3 normal = normalize(fragNor);
	vec3 light = normalize(lightDir);
	vec3 light2 = normalize(lightDir2);
	vec3 light3 = normalize(lightDir3);
	vec3 H = normalize(light + camPos - EPos);
	vec3 H2 = normalize(light2 + camPos - EPos);
	vec3 H3 = normalize(light3 + camPos - EPos);

	// color = vec4(10*MatAmb * max(0, normal.x * light.x + normal.y * light.y + normal.z * light.z), 1.0);

	float nlDot = max(0, normal.x * light.x + normal.y * light.y + normal.z * light.z);
	float nlDot2 = max(0, normal.x * light2.x + normal.y * light2.y + normal.z * light2.z);
	float nlDot3 = max(0, normal.x * light3.x + normal.y * light3.y + normal.z * light3.z);
	float nhDot = max(0, normal.x * H.x + normal.y * H.y + normal.z * H.z);
	float nhDot2 = max(0, normal.x * H2.x + normal.y * H2.y + normal.z * H2.z);
	float nhDot3 = max(0, normal.x * H3.x + normal.y * H3.y + normal.z * H3.z);

	float I_e = 0;
	float I_aL = 3;
	float I_dL = 1;
	float I_sL = 1;
	
	vec4 color1 = I_e + vec4(MatAmb*I_aL + MatDif*nlDot*I_dL + MatSpec*pow(nhDot, MatShine)*I_sL, 1.0);
	vec4 color2 = I_e + vec4(MatAmb*I_aL + MatDif*nlDot2*I_dL + MatSpec*pow(nhDot2, MatShine)*I_sL, 1.0);
	vec4 color3 = I_e + vec4(MatAmb*I_aL + MatDif*nlDot3*I_dL + MatSpec*pow(nhDot3, MatShine)*I_sL, 1.0);

	color = max(max(color1, color2), color3);
	// color = vec4(normal, 1);
}
