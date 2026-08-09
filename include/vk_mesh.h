#pragma once

#include <vk_types.h>
#include <vector>
#include <glm/vec3.hpp>

struct VertexInputDescription {
	std::vector<VkVertexInputBindingDescription> bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;

	VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Mesh {
	std::vector<Vertex> _vertices;
	AllocatedBuffer _vertexBuffer;

	std::vector<uint32_t> _indices;
	AllocatedBuffer _indexBuffer;

	bool load_from_obj(const char* filename);
};