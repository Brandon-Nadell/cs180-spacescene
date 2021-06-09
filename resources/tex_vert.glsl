#version  330 core
layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;
uniform mat4 P;
uniform mat4 M;
uniform mat4 V;

out vec2 vTexCoord;
out vec3 fragNor;
out vec3 lightDir;
out vec3 lightDir2;
out vec3 lightDir3;
out vec3 EPos;

uniform vec3 lightPos;
uniform vec3 lightPos2;
uniform vec3 lightPos3;

void main() {

  vec4 vPosition;

  /* First model transforms */
  gl_Position = P * V *M * vec4(vertPos.xyz, 1.0);

  fragNor = (M * vec4(vertNor, 0.0)).xyz;

  /* pass through the texture coordinates to be interpolated */
  vTexCoord = vertTex;

  lightDir = lightPos - (M*vertPos).xyz;
  lightDir2 = lightPos2 - (M*vertPos).xyz;
  lightDir3 = lightPos3 - (M*vertPos).xyz;
  EPos = (M*vertPos).xyz;
}
