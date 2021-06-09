#version 330 core
uniform sampler2D Texture0;
// uniform sampler2D NormalMap;
uniform vec3 camPos;

in vec2 vTexCoord;
out vec4 Outcolor;

in vec3 fragNor;
in vec3 lightDir;
in vec3 lightDir2;
//position of the vertex in camera space
in vec3 EPos;

void main() {
  	vec4 texColor0 = texture(Texture0, vTexCoord);
	Outcolor = texColor0;
}

