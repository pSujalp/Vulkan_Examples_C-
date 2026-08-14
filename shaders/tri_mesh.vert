#version 450
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 3) in vec2 vUV;





layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 lightPos;
layout (location = 4) out vec3 camPos;
layout (location = 5) out vec3 outFragPos;
layout (location = 6) out vec3 outNor;


//push constants block
layout( push_constant ) uniform constants
{
	vec4 data;
	mat4 render_matrix;
	vec3 lightPos;
	vec3 cameraPos;
} PushConstants;

void main()
{
	gl_Position = PushConstants.render_matrix * vec4(vPosition, 1.0f);
	outUV = vUV;
	lightPos =PushConstants.lightPos;
	camPos = PushConstants.cameraPos ;
	outFragPos = vPosition;
	outNor = vNormal;

}