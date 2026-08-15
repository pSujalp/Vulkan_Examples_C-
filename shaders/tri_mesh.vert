#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 4) out vec3 inDir;

layout(set = 0, binding = 0) uniform  CameraBuffer{
	mat4 mvp;
} cameraData;


layout(set = 1, binding = 1) uniform GPUSCENE{
	vec4 fogColor; 
} gpuScene;


layout( push_constant ) uniform constants
{
	vec4 data;
	mat4 render_matrix;
} PushConstants;



void main()
{
	gl_Position = (cameraData.mvp * vec4(vPosition, 1.0f)).xyww;
	inDir = (vPosition);
}