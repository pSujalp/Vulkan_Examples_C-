#include <vk_textures.h>
#include <vk_engine.h>
#include <iostream>

#include <vk_initializers.h>

#include <stb_image.h>

bool vkutil::load_image_from_file(VulkanEngine &engine, const char *file, AllocatedImage &outImage)
{

	stbi_set_flip_vertically_on_load(true);
	int texWidth, texHeight, texChannels;

	stbi_uc *pixels = stbi_load(file, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
	{
		std::cout << "Failed to load texture file " << file << std::endl;
		return false;
	}

	void *pixel_ptr = pixels;
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	VkFormat image_format = VK_FORMAT_R8G8B8A8_SRGB;
	AllocatedBuffer stagingBuffer = engine.create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
	void *data;
	vmaMapMemory(engine._allocator, stagingBuffer._allocation, &data);
	memcpy(data, pixel_ptr, static_cast<size_t>(imageSize));
	vmaUnmapMemory(engine._allocator, stagingBuffer._allocation);

	stbi_image_free(pixels);

	VkExtent3D imageExtent;
	imageExtent.width = static_cast<uint32_t>(texWidth);
	imageExtent.height = static_cast<uint32_t>(texHeight);
	imageExtent.depth = 1;

	VkImageCreateInfo dimg_info = vkinit::image_create_info(image_format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);

	AllocatedImage newImage;

	VmaAllocationCreateInfo dimg_allocinfo = {};
	dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	vmaCreateImage(engine._allocator, &dimg_info, &dimg_allocinfo, &newImage.image, &newImage.allocation, nullptr);

	VkCommandPoolCreateInfo uploadPoolInfo = vkinit::command_pool_create_info(engine._graphicsQueueFamily);
	VkCommandPool uploadPool;
	VK_CHECK(vkCreateCommandPool(engine._device, &uploadPoolInfo, nullptr, &uploadPool));

	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(uploadPool, 1);
	VkCommandBuffer cmd;
	VK_CHECK(vkAllocateCommandBuffers(engine._device, &cmdAllocInfo, &cmd));

	VkCommandBufferBeginInfo beginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

	VkImageSubresourceRange range;
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 1;

	VkImageMemoryBarrier imageBarrier_toTransfer = {};
	imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageBarrier_toTransfer.image = newImage.image;
	imageBarrier_toTransfer.subresourceRange = range;
	imageBarrier_toTransfer.srcAccessMask = 0;
	imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
						 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

	VkBufferImageCopy copyRegion = {};
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;
	copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.imageSubresource.mipLevel = 0;
	copyRegion.imageSubresource.baseArrayLayer = 0;
	copyRegion.imageSubresource.layerCount = 1;
	copyRegion.imageExtent = imageExtent;

	vkCmdCopyBufferToImage(cmd, stagingBuffer._buffer, newImage.image,
						   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
	imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
						 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = vkinit::submit_info(&cmd);

	VkFenceCreateInfo fenceInfo = vkinit::fence_create_info();
	VkFence uploadFence;
	VK_CHECK(vkCreateFence(engine._device, &fenceInfo, nullptr, &uploadFence));

	VK_CHECK(vkQueueSubmit(engine._graphicsQueue, 1, &submit, uploadFence));

	vkWaitForFences(engine._device, 1, &uploadFence, true, 9999999999);

	vkDestroyFence(engine._device, uploadFence, nullptr);
	vkDestroyCommandPool(engine._device, uploadPool, nullptr);

	engine._mainDeletionQueue.push_function([=]()
											{ vmaDestroyImage(engine._allocator, newImage.image, newImage.allocation); });

	vmaDestroyBuffer(engine._allocator, stagingBuffer._buffer, stagingBuffer._allocation);

	std::cout << "Texture loaded succesfully " << file << std::endl;

	outImage = newImage;
	return true;
}

bool vkutil::load_cubemap_from_files(VulkanEngine &engine, const std::array<std::string, 6> &files, AllocatedImage &outImage)
{
	int texWidth = 0, texHeight = 0, texChannels;
	std::array<unsigned char *, 6> facePixels;

	for (int i = 0; i < 6; i++)
	{
		int w, h;
		facePixels[i] = stbi_load(files[i].c_str(), &w, &h, &texChannels, STBI_rgb_alpha);
		std::cout << files[i] << " -> " << w << "x" << h << std::endl;
		if (!facePixels[i])
		{
			std::cout << "Failed to load cubemap face: " << files[i] << std::endl;
			for (int j = 0; j < i; j++) stbi_image_free(facePixels[j]);
			return false;
		}
		if (i == 0)
		{
			texWidth = w;
			texHeight = h;
		}
	}

	VkDeviceSize faceSize = texWidth * texHeight * 4;
	VkDeviceSize totalSize = faceSize * 6;

	AllocatedBuffer stagingBuffer = engine.create_buffer(totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	void *data;
	vmaMapMemory(engine._allocator, stagingBuffer._allocation, &data);
	for (int i = 0; i < 6; i++)
		memcpy((char *)data + faceSize * i, facePixels[i], faceSize);
	vmaUnmapMemory(engine._allocator, stagingBuffer._allocation);

	for (int i = 0; i < 6; i++)
		stbi_image_free(facePixels[i]);

	VkExtent3D imageExtent{(uint32_t)texWidth, (uint32_t)texHeight, 1};

	VkImageCreateInfo dimg_info = vkinit::image_create_info(
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		imageExtent);
	dimg_info.arrayLayers = 6;
	dimg_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	VmaAllocationCreateInfo dimg_allocinfo = {};
	dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	AllocatedImage newImage;
	vmaCreateImage(engine._allocator, &dimg_info, &dimg_allocinfo, &newImage.image, &newImage.allocation, nullptr);

	VkCommandPoolCreateInfo uploadPoolInfo = vkinit::command_pool_create_info(engine._graphicsQueueFamily);
	VkCommandPool uploadPool;
	VK_CHECK(vkCreateCommandPool(engine._device, &uploadPoolInfo, nullptr, &uploadPool));

	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(uploadPool, 1);
	VkCommandBuffer cmd;
	VK_CHECK(vkAllocateCommandBuffers(engine._device, &cmdAllocInfo, &cmd));

	VkCommandBufferBeginInfo beginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

	// range now covers all 6 layers
	VkImageSubresourceRange range{};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 6;

	VkImageMemoryBarrier imageBarrier_toTransfer = {};
	imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageBarrier_toTransfer.image = newImage.image;
	imageBarrier_toTransfer.subresourceRange = range;
	imageBarrier_toTransfer.srcAccessMask = 0;
	imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
						 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

	// one copy region per face, each targeting its own array layer
	std::array<VkBufferImageCopy, 6> copyRegions{};
	for (int i = 0; i < 6; i++)
	{
		copyRegions[i].bufferOffset = faceSize * i;
		copyRegions[i].bufferRowLength = 0;
		copyRegions[i].bufferImageHeight = 0;
		copyRegions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegions[i].imageSubresource.mipLevel = 0;
		copyRegions[i].imageSubresource.baseArrayLayer = i;
		copyRegions[i].imageSubresource.layerCount = 1;
		copyRegions[i].imageExtent = imageExtent;
	}

	vkCmdCopyBufferToImage(cmd, stagingBuffer._buffer, newImage.image,
						   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						   (uint32_t)copyRegions.size(), copyRegions.data());

	VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
	imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
						 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = vkinit::submit_info(&cmd);

	VkFenceCreateInfo fenceInfo = vkinit::fence_create_info();
	VkFence uploadFence;
	VK_CHECK(vkCreateFence(engine._device, &fenceInfo, nullptr, &uploadFence));
	VK_CHECK(vkQueueSubmit(engine._graphicsQueue, 1, &submit, uploadFence));
	vkWaitForFences(engine._device, 1, &uploadFence, true, 9999999999);
	vkDestroyFence(engine._device, uploadFence, nullptr);
	vkDestroyCommandPool(engine._device, uploadPool, nullptr);

	engine._mainDeletionQueue.push_function([=]()
											{ vmaDestroyImage(engine._allocator, newImage.image, newImage.allocation); });

	vmaDestroyBuffer(engine._allocator, stagingBuffer._buffer, stagingBuffer._allocation);

	std::cout << "Cubemap loaded successfully" << std::endl;

	outImage = newImage;
	return true;
}