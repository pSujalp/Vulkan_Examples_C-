#version 450

layout (location = 0) out vec4 outFragColor;
layout (location = 2) in vec2 inUV;

layout(set = 0, binding = 0) uniform sampler2D tex1;


layout (location = 3) in vec3 lightPos;
layout (location = 4) in vec3 camPos;
layout (location = 5) in vec3 inFragPos;
layout (location = 6) in vec3 inNor;


void main()
{

	vec3 color = texture(tex1, inUV).rgb;

	vec3 ambient = 0.05 * color;

	vec3 lightDir = normalize(lightPos - inFragPos);
	vec3 normal = normalize(inNor);
	float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;
	vec3 viewDir = normalize(camPos - inFragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;

	vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

	vec3 specular = vec3(0.3) * spec;
	
	
	outFragColor = vec4(ambient + diffuse + specular, 1.0);;



}