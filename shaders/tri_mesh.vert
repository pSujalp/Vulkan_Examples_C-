#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

layout (location = 1) out vec3 outColor;

layout(set = 0, binding = 0) uniform  CameraBuffer{
	mat4 mvp;
} cameraData;


layout(set = 1, binding = 1) uniform GPUSCENE{
	vec4 fogColor; // w is for exponent
	vec4 fogDistances; //x for min, y for max, zw unused.
	vec4 ambientColor;
	vec4 sunlightDirection; //w for sun power
	vec4 sunlightColor;
} gpuScene;


layout( push_constant ) uniform constants
{
	vec4 data;
	mat4 render_matrix;
} PushConstants;



void main()
{
	gl_Position = cameraData.mvp * vec4(vPosition, 1.0f);
	outColor = vColor;
}