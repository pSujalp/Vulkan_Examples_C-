#version 450
layout (location = 0) out vec4 outFragColor;

layout (location = 4) in vec3 indDir;
layout(set = 2, binding = 0) uniform samplerCube equirectangularMap;

void main()
{
	vec3 dir = normalize(vec3(indDir.x, indDir.y, -indDir.z));
	outFragColor = texture(equirectangularMap, dir);
}