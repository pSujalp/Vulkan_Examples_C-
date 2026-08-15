#pragma once

#include <vk_types.h>

class VulkanEngine;

namespace vkutil {

	bool load_image_from_file(VulkanEngine&engine, const char* file, AllocatedImage& outImage);

	bool load_cubemap_from_files(VulkanEngine& engine, const std::array<std::string, 6>& files, AllocatedImage& outImage);
}