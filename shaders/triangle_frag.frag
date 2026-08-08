#version 450

//shader input
layout (location = 1) in vec3 inColor;

//output write
layout (location = 0) out vec4 outFragColor;


layout (location = 2) in vec2 inUV;


void main()
{
	
	outFragColor = vec4(inUV.x, inUV.y, 0.5f, 1.0f);
}