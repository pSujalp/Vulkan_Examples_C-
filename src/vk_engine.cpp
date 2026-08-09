
#include "vk_engine.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_types.h>
#include <vk_initializers.h>

#include "VkBootstrap.h"

#include <iostream>

constexpr bool bUseValidationLayers = true;
using namespace std;

void VulkanEngine::init()
{

	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

	_window = SDL_CreateWindow(
		"Vulkan Engine",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		_windowExtent.width,
		_windowExtent.height,
		window_flags);
	init_vulkan();

	init_swapchain();

	init_default_renderpass();

	init_framebuffers();

	init_commands();

	init_sync_structures();

	init_descriptor();

	init_pipelines();

	load_meshes();

	_isInitialized = true;
}
void VulkanEngine::cleanup()
{
	if (_isInitialized)
	{

		vkDeviceWaitIdle(_device);

		vkDestroyCommandPool(_device, _commandPool, nullptr);

		vmaDestroyAllocator(_allocator);

		vkDestroyFence(_device, _renderFence, nullptr);
		vkDestroySemaphore(_device, _renderSemaphore, nullptr);
		vkDestroySemaphore(_device, _presentSemaphore, nullptr);

		vkDestroySwapchainKHR(_device, _swapchain, nullptr);

		vkDestroyRenderPass(_device, _renderPass, nullptr);

		for (int i = 0; i < _framebuffers.size(); i++)
		{
			vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);

			vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
		}

		vkDestroySurfaceKHR(_instance, _surface, nullptr);

		_mainDeletionQueue.flush();

		vkDestroyDevice(_device, nullptr);
		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
		vkDestroyInstance(_instance, nullptr);

		SDL_DestroyWindow(_window);
	}
}

void VulkanEngine::draw()
{

	if (SDL_GetWindowFlags(_window) & SDL_WINDOW_MINIMIZED)
		return;

	VK_CHECK(vkWaitForFences(_device, 1, &_renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(_device, 1, &_renderFence));

	VK_CHECK(vkResetCommandBuffer(_mainCommandBuffer, 0));

	uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, _presentSemaphore, nullptr, &swapchainImageIndex));

	VkCommandBuffer cmd = _mainCommandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	VkClearValue clearValue;

	clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

	VkRenderPassBeginInfo rpInfo = vkinit::renderpass_begin_info(_renderPass, _windowExtent, _framebuffers[swapchainImageIndex]);

	rpInfo.clearValueCount = 1;
	rpInfo.pClearValues = &clearValue;

	vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

	if (TextureDescriptor != VK_NULL_HANDLE)
	{
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipelineLayout, 0, 1, &TextureDescriptor, 0, nullptr);
	}

	if (TextureDescriptor1 != VK_NULL_HANDLE)
	{
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipelineLayout, 1, 1, &TextureDescriptor1, 0, nullptr);
	}

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipeline);

	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(cmd, 0, 1, &_triangleMesh._vertexBuffer._buffer, &offset);

	vkCmdBindIndexBuffer(cmd, _triangleMesh._indexBuffer._buffer, 0, VK_INDEX_TYPE_UINT32);

	glm::vec3 camPos = {0.f, 0.f, -2.f};

	glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);

	glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
	projection[1][1] *= -1;

	glm::mat4 model = glm::rotate(glm::mat4{1.0f}, glm::radians(_frameNumber * 0.4f), glm::vec3(0, 1, 0));

	glm::mat4 mesh_matrix = projection * view * model;

	MeshPushConstants constants;
	constants.render_matrix = mesh_matrix;

	vkCmdPushConstants(cmd, _meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

	vkCmdDrawIndexed(cmd, static_cast<uint32_t>(_triangleMesh._indices.size()), 1, 0, 0, 0);

	vkCmdEndRenderPass(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = vkinit::submit_info(&cmd);
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	submit.pWaitDstStageMask = &waitStage;

	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &_presentSemaphore;

	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &_renderSemaphore;

	VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, _renderFence));

	VkPresentInfoKHR presentInfo = vkinit::present_info();

	presentInfo.pSwapchains = &_swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &_renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

	_frameNumber++;
}

void VulkanEngine::run()
{
	SDL_Event e;
	bool bQuit = false;

	while (!bQuit)
	{

		while (SDL_PollEvent(&e) != 0)
		{

			if (e.type == SDL_QUIT)
				bQuit = true;
		}

		draw();
	}
}

void VulkanEngine::init_vulkan()
{
	vkb::InstanceBuilder builder;

	auto inst_ret = builder.set_app_name("Example Vulkan Application")
						.request_validation_layers(bUseValidationLayers)
						.use_default_debug_messenger()
						.require_api_version(1, 1, 0)
						.build();

	vkb::Instance vkb_inst = inst_ret.value();

	_instance = vkb_inst.instance;
	_debug_messenger = vkb_inst.debug_messenger;

	SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

	vkb::PhysicalDeviceSelector selector{vkb_inst};
	vkb::PhysicalDevice physicalDevice = selector
											 .set_minimum_version(1, 1)
											 .set_surface(_surface)
											 .select()
											 .value();

	vkb::DeviceBuilder deviceBuilder{physicalDevice};

	vkb::Device vkbDevice = deviceBuilder.build().value();

	_device = vkbDevice.device;
	_chosenGPU = physicalDevice.physical_device;

	_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();

	_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = _chosenGPU;
	allocatorInfo.device = _device;
	allocatorInfo.instance = _instance;
	vmaCreateAllocator(&allocatorInfo, &_allocator);
}

void VulkanEngine::init_swapchain()
{
	vkb::SwapchainBuilder swapchainBuilder{_chosenGPU, _device, _surface};

	vkb::Swapchain vkbSwapchain = swapchainBuilder
									  .use_default_format_selection()

									  .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
									  .set_desired_extent(_windowExtent.width, _windowExtent.height)
									  .build()
									  .value();

	_swapchain = vkbSwapchain.swapchain;
	_swapchainImages = vkbSwapchain.get_images().value();
	_swapchainImageViews = vkbSwapchain.get_image_views().value();

	_swachainImageFormat = vkbSwapchain.image_format;

	_mainDeletionQueue.push_function([=]()
									 { vkDestroySwapchainKHR(_device, _swapchain, nullptr); });
}

void VulkanEngine::init_default_renderpass()
{

	VkAttachmentDescription color_attachment = {};
	color_attachment.format = _swachainImageFormat;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref = {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &color_attachment;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;

	VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_renderPass));

	_mainDeletionQueue.push_function([=]()
									 { vkDestroyRenderPass(_device, _renderPass, nullptr); });
}

void VulkanEngine::init_framebuffers()
{

	VkFramebufferCreateInfo fb_info = vkinit::framebuffer_create_info(_renderPass, _windowExtent);

	const uint32_t swapchain_imagecount = _swapchainImages.size();
	_framebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

	for (int i = 0; i < swapchain_imagecount; i++)
	{

		fb_info.pAttachments = &_swapchainImageViews[i];
		VK_CHECK(vkCreateFramebuffer(_device, &fb_info, nullptr, &_framebuffers[i]));
		_mainDeletionQueue.push_function([=]()
										 {
			vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);
			vkDestroyImageView(_device, _swapchainImageViews[i], nullptr); });
	}
}

void VulkanEngine::init_commands()
{

	VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_commandPool));

	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_commandPool, 1);

	VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_mainCommandBuffer));

	_mainDeletionQueue.push_function([=]()
									 { vkDestroyCommandPool(_device, _commandPool, nullptr); });
}

void VulkanEngine::init_sync_structures()
{
	VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

	VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_renderFence));

	_mainDeletionQueue.push_function([=]()
									 { vkDestroyFence(_device, _renderFence, nullptr); });

	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

	VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_presentSemaphore));
	VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_renderSemaphore));

	_mainDeletionQueue.push_function([=]()
									 {
        vkDestroySemaphore(_device, _presentSemaphore, nullptr);
        vkDestroySemaphore(_device, _renderSemaphore, nullptr); });
}

bool VulkanEngine::load_shader_module(const char *filePath, VkShaderModule *outShaderModule)
{

	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		return false;
	}

	size_t fileSize = (size_t)file.tellg();

	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	file.seekg(0);

	file.read((char *)buffer.data(), fileSize);

	file.close();

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;

	createInfo.codeSize = buffer.size() * sizeof(uint32_t);
	createInfo.pCode = buffer.data();

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		return false;
	}
	*outShaderModule = shaderModule;
	return true;
}

void VulkanEngine::init_pipelines()
{

	VkShaderModule triangleFragShader;
	if (!load_shader_module("shaders/triangle_frag.frag.spv", &triangleFragShader))
	{
		std::cout << "Error when building the triangle fragment shader module" << std::endl;
	}
	else
	{
		std::cout << "Triangle fragment shader successfully loaded" << std::endl;
	}

	VkShaderModule triangleVertexShader;
	if (!load_shader_module("shaders/triangle_vert.vert.spv", &triangleVertexShader))
	{
		std::cout << "Error when building the triangle vertex shader module" << std::endl;
	}
	else
	{
		std::cout << "Triangle vertex shader successfully loaded" << std::endl;
	}

	VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();

	VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_trianglePipelineLayout));

	PipelineBuilder pipelineBuilder;

	pipelineBuilder._shaderStages.push_back(
		vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, triangleVertexShader));

	pipelineBuilder._shaderStages.push_back(
		vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));

	pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();

	pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	pipelineBuilder._viewport.x = 0.0f;
	pipelineBuilder._viewport.y = 0.0f;
	pipelineBuilder._viewport.width = (float)_windowExtent.width;
	pipelineBuilder._viewport.height = (float)_windowExtent.height;
	pipelineBuilder._viewport.minDepth = 0.0f;
	pipelineBuilder._viewport.maxDepth = 1.0f;

	pipelineBuilder._scissor.offset = {0, 0};
	pipelineBuilder._scissor.extent = _windowExtent;

	pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);

	pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();

	pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();

	pipelineBuilder._pipelineLayout = _trianglePipelineLayout;

	_trianglePipeline = pipelineBuilder.build_pipeline(_device, _renderPass);

	VertexInputDescription vertexDescription = Vertex::get_vertex_description();

	pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
	pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();

	pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
	pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

	pipelineBuilder._shaderStages.clear();

	VkShaderModule meshVertShader;
	if (!load_shader_module("shaders/tri_mesh.vert.spv", &meshVertShader))
	{
		std::cout << "Error when building the triangle vertex shader module" << std::endl;
	}
	else
	{
		std::cout << "Red Triangle vertex shader successfully loaded" << std::endl;
	}

	pipelineBuilder._shaderStages.push_back(
		vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, meshVertShader));

	pipelineBuilder._shaderStages.push_back(
		vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));

	VkDescriptorSetLayout layouts[] = {
		_singleTextureSetLayout,
		_mixsingleTextureSetLayout};

	VkPipelineLayoutCreateInfo mesh_pipeline_layout_info = {};
	mesh_pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	mesh_pipeline_layout_info.pNext = nullptr;
	mesh_pipeline_layout_info.setLayoutCount = 2;
	mesh_pipeline_layout_info.pSetLayouts = layouts;
	mesh_pipeline_layout_info.pushConstantRangeCount = 1;
	VkPushConstantRange push_constant;
	push_constant.offset = 0;
	push_constant.size = sizeof(MeshPushConstants);
	push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	mesh_pipeline_layout_info.pPushConstantRanges = &push_constant;
	mesh_pipeline_layout_info.pushConstantRangeCount = 1;

	VK_CHECK(vkCreatePipelineLayout(_device, &mesh_pipeline_layout_info, nullptr, &_meshPipelineLayout));

	pipelineBuilder._pipelineLayout = _meshPipelineLayout;
	_meshPipeline = pipelineBuilder.build_pipeline(_device, _renderPass);

	vkDestroyShaderModule(_device, triangleFragShader, nullptr);
	vkDestroyShaderModule(_device, triangleVertexShader, nullptr);
	vkDestroyShaderModule(_device, meshVertShader, nullptr);

	_mainDeletionQueue.push_function([=]()
									 {
        vkDestroyPipeline(_device, _trianglePipeline, nullptr);
		vkDestroyPipeline(_device, _meshPipeline, nullptr);

		vkDestroyPipelineLayout(_device, _trianglePipelineLayout, nullptr); });

	_mainDeletionQueue.push_function([=]()
									 { vkDestroyPipelineLayout(_device, _meshPipelineLayout, nullptr); });
}

void VulkanEngine::load_meshes()
{

	_triangleMesh._vertices.resize(4);

	_triangleMesh._vertices[0].position = glm::vec3{0.5f, 0.5f, 0.0f};
	_triangleMesh._vertices[1].position = glm::vec3{0.5f, -0.5f, 0.0f};
	_triangleMesh._vertices[2].position = glm::vec3{-0.5f, -0.5f, 0.0f};
	_triangleMesh._vertices[3].position = glm::vec3{-0.5f, 0.5f, 0.0f};

	_triangleMesh._vertices[0].color = glm::vec3{1.0f, 0.0f, 0.0f};
	_triangleMesh._vertices[1].color = glm::vec3{0.0f, 1.0f, 0.0f};
	_triangleMesh._vertices[2].color = glm::vec3{0.0f, 0.0f, 1.0f};
	_triangleMesh._vertices[3].color = glm::vec3{1.0f, 1.0f, 0.0f};

	_triangleMesh._vertices[0].uv = glm::vec2{1.0f, 1.0f};
	_triangleMesh._vertices[1].uv = glm::vec2{1.0f, 0.0f};
	_triangleMesh._vertices[2].uv = glm::vec2{0.0f, 0.0f};
	_triangleMesh._vertices[3].uv = glm::vec2{0.0f, 1.0f};

	_triangleMesh._indices.resize(6);
	_triangleMesh._indices[0] = 0;
	_triangleMesh._indices[1] = 1;
	_triangleMesh._indices[2] = 3;
	_triangleMesh._indices[3] = 1;
	_triangleMesh._indices[4] = 2;
	_triangleMesh._indices[5] = 3;

	upload_mesh(_triangleMesh);
}

void VulkanEngine::upload_mesh(Mesh &mesh)
{

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

	bufferInfo.size = mesh._vertices.size() * sizeof(Vertex);

	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo,
					&mesh._vertexBuffer._buffer,
					&mesh._vertexBuffer._allocation,
					nullptr);

	_mainDeletionQueue.push_function([=]()
									 { vmaDestroyBuffer(_allocator, mesh._vertexBuffer._buffer, mesh._vertexBuffer._allocation); });

	void *data;
	vmaMapMemory(_allocator, mesh._vertexBuffer._allocation, &data);

	memcpy(data, mesh._vertices.data(), mesh._vertices.size() * sizeof(Vertex));

	vmaUnmapMemory(_allocator, mesh._vertexBuffer._allocation);

	data = nullptr;

	VkBufferCreateInfo indexBufferInfo = {};
	indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	indexBufferInfo.size = mesh._indices.size() * sizeof(uint32_t);
	indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	VmaAllocationCreateInfo indexAllocInfo = {};
	indexAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	VK_CHECK(vmaCreateBuffer(_allocator, &indexBufferInfo, &indexAllocInfo,
							 &mesh._indexBuffer._buffer,
							 &mesh._indexBuffer._allocation,
							 nullptr));

	vmaMapMemory(_allocator, mesh._indexBuffer._allocation, &data);
	memcpy(data, mesh._indices.data(), mesh._indices.size() * sizeof(uint32_t));
	vmaUnmapMemory(_allocator, mesh._indexBuffer._allocation);

	_mainDeletionQueue.push_function([=]()
									 { vmaDestroyBuffer(_allocator, mesh._indexBuffer._buffer, mesh._indexBuffer._allocation); });
}

AllocatedBuffer VulkanEngine::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;
	bufferInfo.size = allocSize;
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memoryUsage;

	AllocatedBuffer newBuffer;

	VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo,
							 &newBuffer._buffer,
							 &newBuffer._allocation,
							 nullptr));

	return newBuffer;
}

void VulkanEngine::init_descriptor()
{

	std::vector<VkDescriptorPoolSize> sizes =
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10}};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = 0;
	pool_info.maxSets = 10;
	pool_info.poolSizeCount = (uint32_t)sizes.size();
	pool_info.pPoolSizes = sizes.data();

	vkCreateDescriptorPool(_device, &pool_info, nullptr, &_descriptorPool);

	VkDescriptorSetLayoutBinding textureBind = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

	VkDescriptorSetLayoutCreateInfo set3info = {};
	set3info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set3info.bindingCount = 1;
	set3info.flags = 0;
	set3info.pNext = nullptr;
	set3info.pBindings = &textureBind;

	vkCreateDescriptorSetLayout(_device, &set3info, nullptr, &_singleTextureSetLayout);

	VkDescriptorSetLayoutBinding textureBind1 = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	VkDescriptorSetLayoutCreateInfo set4info = {};
	set4info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set4info.bindingCount = 1;
	set4info.flags = 0;
	set4info.pNext = nullptr;
	set4info.pBindings = &textureBind1;
	vkCreateDescriptorSetLayout(_device, &set4info, nullptr, &_mixsingleTextureSetLayout);
	VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST);
	VK_CHECK(vkCreateSampler(_device, &samplerInfo, nullptr, &_blockySampler));



	_mainDeletionQueue.push_function([=]()
									 { vkDestroySampler(_device, _blockySampler, nullptr);
									    vkDestroyDescriptorSetLayout(_device, _mixsingleTextureSetLayout, nullptr); 
									 vkDestroyDescriptorSetLayout(_device, _singleTextureSetLayout, nullptr); });

	bool texLoaded = vkutil::load_image_from_file(*this, "assets/container.jpg", _texture);
	bool texLoaded1 = vkutil::load_image_from_file(*this, "assets/awesomeface.png", _texture1);

	if (texLoaded && texLoaded1)
	{
		VkImageViewCreateInfo imageInfo = vkinit::imageview_create_info(VK_FORMAT_R8G8B8A8_SRGB, _texture.image, VK_IMAGE_ASPECT_COLOR_BIT);
		VK_CHECK(vkCreateImageView(_device, &imageInfo, nullptr, &_textureImageView));

		_mainDeletionQueue.push_function([=]()
										 { vkDestroyImageView(_device, _textureImageView, nullptr); }
										);

		VkDescriptorSetAllocateInfo texAllocInfo = {};
		texAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		texAllocInfo.pNext = nullptr;
		texAllocInfo.descriptorPool = _descriptorPool;
		texAllocInfo.descriptorSetCount = 1;
		texAllocInfo.pSetLayouts = &_singleTextureSetLayout;

		VK_CHECK(vkAllocateDescriptorSets(_device, &texAllocInfo, &TextureDescriptor));

		VkDescriptorImageInfo imageBufferInfo = {};
		imageBufferInfo.sampler = _blockySampler;
		imageBufferInfo.imageView = _textureImageView;
		imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet Writetexture0 = {};
		Writetexture0.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		Writetexture0.pNext = nullptr;
		Writetexture0.dstBinding = 0;
		Writetexture0.dstSet = TextureDescriptor;
		Writetexture0.descriptorCount = 1;
		Writetexture0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		Writetexture0.pImageInfo = &imageBufferInfo;

		imageInfo = vkinit::imageview_create_info(VK_FORMAT_R8G8B8A8_SRGB, _texture1.image, VK_IMAGE_ASPECT_COLOR_BIT);
		VK_CHECK(vkCreateImageView(_device, &imageInfo, nullptr, &_textureImageView1));

		_mainDeletionQueue.push_function([=]()
										 { vkDestroyImageView(_device, _textureImageView1, nullptr); });

		VkDescriptorSetAllocateInfo texAllocInfo1 = {};
		texAllocInfo1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		texAllocInfo1.pNext = nullptr;
		texAllocInfo1.descriptorPool = _descriptorPool;
		texAllocInfo1.descriptorSetCount = 1;
		texAllocInfo1.pSetLayouts = &_mixsingleTextureSetLayout;

		VK_CHECK(vkAllocateDescriptorSets(_device, &texAllocInfo1, &TextureDescriptor1));

		VkDescriptorImageInfo imageBufferInfo1 = {};
		imageBufferInfo1.sampler = _blockySampler;
		imageBufferInfo1.imageView = _textureImageView1;
		imageBufferInfo1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet Writetexture1 = {};
		Writetexture1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		Writetexture1.pNext = nullptr;
		Writetexture1.dstBinding = 1;
		Writetexture1.dstSet = TextureDescriptor1;
		Writetexture1.descriptorCount = 1;
		Writetexture1.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		Writetexture1.pImageInfo = &imageBufferInfo1;

		VkWriteDescriptorSet setWrites[] = {Writetexture0, Writetexture1};

		vkUpdateDescriptorSets(_device, 2, setWrites, 0, nullptr);
	}

	else
	{
		std::cout << "Failed to load texture, set 2 will not be bound in draw()" << std::endl;
		TextureDescriptor = VK_NULL_HANDLE;

		std::cout << "Failed to load texture, set 2 will not be bound in draw()" << std::endl;
		TextureDescriptor1 = VK_NULL_HANDLE;
	}
}