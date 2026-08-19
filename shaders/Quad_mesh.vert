#version 450
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 vNormal;
layout (location = 3) in vec2 vUV;

layout (location = 2) out vec2 outUV;


void main()
{
	gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
	
	outUV = vUV;
}