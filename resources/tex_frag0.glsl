#version 330 core
uniform sampler2D Texture0;
uniform sampler2D NormalMap;
uniform vec3 camPos;

in vec2 vTexCoord;
out vec4 Outcolor;

in vec3 fragNor;
in vec3 lightDir;
in vec3 lightDir2;
in vec3 lightDir3;
//position of the vertex in camera space
in vec3 EPos;

void main() {
	// obtain normal from normal map in range [0,1]
    vec3 norm = texture(NormalMap, vTexCoord).rgb;
    // transform normal vector to range [-1,1]
    norm = normalize(norm * 2.0 - 1.0);  // this normal is in tangent space

  	vec4 texColor0 = texture(Texture0, vTexCoord);

	vec3 fragNorm = normalize(fragNor);
	vec3 normal = fragNorm;
	vec3 light = normalize(lightDir);
	vec3 light2 = normalize(lightDir2);
	vec3 light3 = normalize(lightDir3);
	vec3 H = normalize(light + camPos - EPos);
	vec3 H2 = normalize(light2 + camPos - EPos);
	vec3 H3 = normalize(light3 + camPos - EPos);

	float nlDot = max(0, normal.x * light.x + normal.y * light.y + normal.z * light.z);
	float nlDot2 = max(0, normal.x * light2.x + normal.y * light2.y + normal.z * light2.z);
	float nlDot3 = max(0, normal.x * light3.x + normal.y * light3.y + normal.z * light3.z);
	float nhDot = max(0, normal.x * H.x + normal.y * H.y + normal.z * H.z);
	float nhDot2 = max(0, normal.x * H2.x + normal.y * H2.y + normal.z * H2.z);
	float nhDot3 = max(0, normal.x * H3.x + normal.y * H3.y + normal.z * H3.z);

  	//to set the out color as the texture color 
  	vec4 Outcolor1 = texColor0/6.0 + texColor0*nlDot + texColor0/10.0*pow(nhDot, 4);
  	vec4 Outcolor2 = texColor0/6.0 + texColor0*nlDot2 + texColor0/10.0*pow(nhDot2, 4);
  	vec4 Outcolor3 = texColor0/6.0 + texColor0*nlDot3 + texColor0/10.0*pow(nhDot3, 4);

  	Outcolor = max(max(Outcolor1, Outcolor2), Outcolor3);
  
  	//to set the outcolor as the texture coordinate (for debugging)
	//Outcolor = vec4(vTexCoord.s, vTexCoord.t, 0, 1);

}

