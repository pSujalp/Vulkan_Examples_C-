#pragma once 
#include <vk_types.h>
#include <vk_mesh.h>


struct Material
{
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

struct RenderObject
{
	Mesh *mesh;

	Material *material;

	glm::mat4 transformMatrix;
};