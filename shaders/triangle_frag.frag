#version 450

//shader input
layout (location = 1) in vec3 inColor;

//output write
layout (location = 0) out vec4 outFragColor;


layout (location = 2) in vec2 inUV;


layout(set = 2, binding = 0) uniform sampler2D tex1;


void main()
{
	
	const vec3 color = texture(tex1,inUV).xyz;
	outFragColor = vec4(color,1.0f);
}