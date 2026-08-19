#version 450
layout (location = 0) out vec4 outFragColor;
layout (location = 2) in vec2 inUV;

layout(set = 0, binding = 0) uniform sampler2D tex1;

void main()
{	vec4 FragColor = texture(tex1, inUV);
	float average = (FragColor.r + FragColor.g + FragColor.b) / 3.0;
	outFragColor = vec4(average, average, average, 1.0);

}