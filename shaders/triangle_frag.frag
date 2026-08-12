#version 450

layout (location = 0) out vec4 outFragColor;
layout (location = 2) in vec2 inUV;

layout(set = 0, binding = 0) uniform sampler2D tex1;



void main()
{
	
	
	outFragColor = texture(tex1, inUV);



}