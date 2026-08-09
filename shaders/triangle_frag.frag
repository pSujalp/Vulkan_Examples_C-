#version 450

//shader input
layout (location = 1) in vec3 inColor;

//output write
layout (location = 0) out vec4 outFragColor;

layout (location = 2) in vec2 inUV;



layout(set = 0, binding = 0) uniform sampler2D tex1;


layout(set = 1, binding = 1) uniform sampler2D tex2;


void main()
{
	
	
	outFragColor = mix(texture(tex1, inUV), texture(tex2, inUV), 0.2) * vec4(inColor,1.0f);



}