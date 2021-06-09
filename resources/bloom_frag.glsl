#version 330 core
// layout (location = 0) out vec4 FragColor;
// layout (location = 1) out vec4 BrightColor;

out vec4 color;

//interpolated normal and light vector in camera space
in vec3 fragNor;
in vec3 lightDir;

void main()
{
	// //you will need to work with these for lighting
	vec3 normal = normalize(fragNor);
	vec3 light = normalize(lightDir);
	// vec3 H = normalize(light + camPos - EPos);

	// vec3 Ncolor = 0.5*normal + vec3(1, .3, -.2);
	// vec4 FragColor = vec4(Ncolor, 1.0);
	vec4 FragColor = vec4(light, 1.0);

    // // check whether fragment output is higher than threshold, if so output as brightness color
	float brightness = FragColor.x * .2126 + FragColor.y * .7152 + FragColor.z * .0722;
    // float brightness = dot(FragColor.xyz, vec3(0.2126, 0.7152, 0.0722));
    if (2 > 1.0)
        color = vec4(FragColor.xyz, 1.0);
    else
        color = vec4(0.0, 0.0, 0.0, 1.0);
}
