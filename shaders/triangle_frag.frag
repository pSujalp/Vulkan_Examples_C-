#version 450

layout (location = 0) out vec4 outFragColor;


layout (location = 3) in vec4 inPos;
layout (location = 4) in vec3 inDir;


layout(set = 2, binding = 0) uniform samplerCube equirectangularMap;


const float M_PI_F = 3.14159265359;

void main()
{
	// Convert direction vector to spherical UV coordinates for equirectangular mapping
	vec3 d = normalize(inDir);
	float u = atan(d.z, d.x) / (2.0 * M_PI_F) + 0.5;
	float v = asin(clamp(d.y, -1.0f, 1.0f)) / M_PI_F + 0.5;

	outFragColor = texture(equirectangularMap, vec3(inPos));
} 