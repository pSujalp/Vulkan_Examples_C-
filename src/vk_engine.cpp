#include "vk_engine.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_types.h>
#include <vk_initializers.h>
#include <vk_textures.h>

#include "VkBootstrap.h"

#include <iostream>

constexpr bool bUseValidationLayers = true;
using namespace std;

FrameData &VulkanEngine::get_current_frame()
{
	return _frames[_frameNumber % FRAME_OVERLAP];
}

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
	init_descriptors();
	init_pipelines();
	load_meshes();

	camera = Camera(glm::vec3(0.f, 0.f, 2.f));

	_isInitialized = true;
}

void VulkanEngine::cleanup()
{
	if (_isInitialized)
	{

		vkDeviceWaitIdle(_device);

		_mainDeletionQueue.flush();

		vkDestroySwapchainKHR(_device, _swapchain, nullptr);

		vkDestroySurfaceKHR(_instance, _surface, nullptr);

		vmaDestroyAllocator(_allocator);

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

	VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

	VK_CHECK(vkResetCommandBuffer(get_current_frame()._mainCommandBuffer, 0));

	uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._presentSemaphore, nullptr, &swapchainImageIndex));

	VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	VkClearValue depthClear;
	VkClearValue clearValue;
	depthClear.depthStencil.depth = 1.f;
	clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

	VkClearValue clearValues[] = {clearValue, depthClear};

	VkRenderPassBeginInfo rpInfo = vkinit::renderpass_begin_info(_renderPass, _windowExtent, _framebuffers[swapchainImageIndex]);
	rpInfo.clearValueCount = 2;
	rpInfo.pClearValues = &clearValues[0];

	vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipeline);

	uint32_t frameIndex = _frameNumber % FRAME_OVERLAP;

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipelineLayout, 0, 1, &CameraDescriptor, 0, nullptr);

	uint32_t uniform_offset = (uint32_t)pad_uniform_buffer_size(sizeof(GPUSceneData1)) * frameIndex;
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipelineLayout, 1, 1, &GPUDescriptor, 1, &uniform_offset);

	if (TextureDescriptor != VK_NULL_HANDLE)
	{
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipelineLayout, 2, 1, &TextureDescriptor, 0, nullptr);
	}

	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(cmd, 0, 1, &_triangleMesh._vertexBuffer._buffer, &offset);

	glm::vec3 camPos = {0.f, 0.f, -2.f};
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 projection = glm::perspective( glm::radians(camera.Zoom),(float)_windowExtent.width / (float)_windowExtent.height,0.1f, 200.0f);
	projection[1][1] *= -1;
	glm::mat4 mesh_matrix = projection * glm::mat4(glm::mat3(view));

	MeshPushConstants constants;
	constants.render_matrix = mesh_matrix;

	vkCmdPushConstants(cmd, _meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

	GPUCameraData camData;
	camData.mvp = mesh_matrix;

	void *data;
	vmaMapMemory(_allocator, cameraBuffer._allocation, &data);
	memcpy(data, &camData, sizeof(GPUCameraData));
	vmaUnmapMemory(_allocator, cameraBuffer._allocation);

	GPUSceneData1 camData1{};
	camData1.Color = glm::vec4(1.f, 0.f, 0.f, 1.f);

	char *scenePtr;
	vmaMapMemory(_allocator, GPUParameterBuffer._allocation, (void **)&scenePtr);
	memcpy(scenePtr + uniform_offset, &camData1, sizeof(GPUSceneData1));
	vmaUnmapMemory(_allocator, GPUParameterBuffer._allocation);

	vkCmdDraw(cmd, _triangleMesh._vertices.size(), 1, 0, 0);

	vkCmdEndRenderPass(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = vkinit::submit_info(&cmd);
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	submit.pWaitDstStageMask = &waitStage;

	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &get_current_frame()._presentSemaphore;

	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &get_current_frame()._renderSemaphore;

	VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, get_current_frame()._renderFence));

	VkPresentInfoKHR presentInfo = vkinit::present_info();

	presentInfo.pSwapchains = &_swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

	_frameNumber++;
}

void VulkanEngine::run()
{
	SDL_Event e;
	bool bQuit = false;

	Uint64 NOW = SDL_GetPerformanceCounter();
	Uint64 LAST = 0;
	double deltaTime = 0;
	int mouseX, mouseY;
	Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);

	float lastX = mouseX;
	float lastY = mouseY;

	while (!bQuit)
	{
		LAST = NOW;
		NOW = SDL_GetPerformanceCounter();
		deltaTime = (double)((NOW - LAST) / (double)SDL_GetPerformanceFrequency());

		string str = "Vulkan Tutorial \t\t\t\t\t\t\t\t\t\t\t\t\t\t FPS:" + to_string((int)1 / deltaTime);
		SDL_SetWindowTitle(_window, str.c_str());

		while (SDL_PollEvent(&e) != 0)
		{
			if (e.type == SDL_QUIT)
				bQuit = true;
			switch (e.type)
			{

			case SDL_QUIT:
				bQuit = true;
				break;

			case SDL_MOUSEBUTTONUP:
				if (e.button.button == SDL_BUTTON_LEFT)
				{

					printf("Left mouse button released at position: %d, %d\n", e.button.x, e.button.y);
					SDL_WarpMouseInWindow(_window, _windowExtent.width / 2, _windowExtent.height / 2);
					SDL_ShowCursor(1);
				}
				break;

			case SDL_KEYUP:
				switch (e.key.keysym.sym)
				{
				case SDLK_SPACE:
					printf("Spacebar released.\n");
					break;
				}
			}
		}
		const Uint8 *keystate = SDL_GetKeyboardState(nullptr);
		int mouseX, mouseY;
		mouseState = SDL_GetMouseState(&mouseX, &mouseY);
		bool isLeftInterfaceClick = (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT));
		if (camera.firstMouse)
		{
			lastX = mouseX;
			lastY = mouseY;
			camera.firstMouse = false;
		}
		float xoffset = mouseX - lastX;
		float yoffset = lastY - mouseY;
		lastX = mouseX;
		lastY = mouseY;
		if (isLeftInterfaceClick)
		{
			camera.ProcessMouseMovement(xoffset, yoffset);
			SDL_ShowCursor(0);
			if (keystate[SDL_SCANCODE_W])
				camera.ProcessKeyboard(FORWARD, deltaTime);
			if (keystate[SDL_SCANCODE_S])
				camera.ProcessKeyboard(BACKWARD, deltaTime);
			if (keystate[SDL_SCANCODE_A])
				camera.ProcessKeyboard(LEFT, deltaTime);
			if (keystate[SDL_SCANCODE_D])
				camera.ProcessKeyboard(RIGHT, deltaTime);
		}

		draw();
	}
}

size_t VulkanEngine::pad_uniform_buffer_size(size_t originalSize)
{
	size_t minUboAlignment = _gpuProperties.limits.minUniformBufferOffsetAlignment;
	size_t alignedSize = originalSize;
	if (minUboAlignment > 0)
	{
		alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}
	return alignedSize;
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

	_gpuProperties = vkbDevice.physical_device.properties;

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

	VkExtent3D depthImageExtent = {
		_windowExtent.width,
		_windowExtent.height,
		1};

	// hardcoding the depth format to 32 bit float
	_depthFormat = VK_FORMAT_D32_SFLOAT;

	// the depth image will be an image with the format we selected and Depth Attachment usage flag
	VkImageCreateInfo dimg_info = vkinit::image_create_info(_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

	// for the depth image, we want to allocate it from GPU local memory
	VmaAllocationCreateInfo dimg_allocinfo = {};
	dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// allocate and create the image
	vmaCreateImage(_allocator, &dimg_info, &dimg_allocinfo, &_depthImage.image, &_depthImage.allocation, nullptr);

	// build an image-view for the depth image to use for rendering
	VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(_depthFormat, _depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depthImageView));

	// add to deletion queues
	_mainDeletionQueue.push_function([=]()
									 {
		vkDestroyImageView(_device, _depthImageView, nullptr);
		vmaDestroyImage(_allocator, _depthImage.image, _depthImage.allocation); });
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

	VkAttachmentDescription depth_attachment = {};

	depth_attachment.flags = 0;
	depth_attachment.format = _depthFormat;
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_ref = {};
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	subpass.pDepthStencilAttachment = &depth_attachment_ref;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency depth_dependency = {};
	depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	depth_dependency.dstSubpass = 0;
	depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depth_dependency.srcAccessMask = 0;
	depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency dependencies[2] = {dependency, depth_dependency};

	VkAttachmentDescription attachments[2] = {color_attachment, depth_attachment};

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	render_pass_info.attachmentCount = 2;
	render_pass_info.pAttachments = &attachments[0];
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;

	render_pass_info.dependencyCount = 2;
	render_pass_info.pDependencies = &dependencies[0];

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

		VkImageView attachments[2];
		attachments[0] = _swapchainImageViews[i];
		attachments[1] = _depthImageView;

		fb_info.pAttachments = attachments;
		fb_info.attachmentCount = 2;

		VK_CHECK(vkCreateFramebuffer(_device, &fb_info, nullptr, &_framebuffers[i]));
	}
}

void VulkanEngine::init_commands()
{
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));

		_mainDeletionQueue.push_function([=]()
										 { vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr); });
	}
}

void VulkanEngine::init_sync_structures()
{
	VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));

		_mainDeletionQueue.push_function([=]()
										 { vkDestroyFence(_device, _frames[i]._renderFence, nullptr); });

		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._presentSemaphore));
		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));

		_mainDeletionQueue.push_function([=]()
										 {
            vkDestroySemaphore(_device, _frames[i]._presentSemaphore, nullptr);
            vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr); });
	}
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
	pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, false, VK_COMPARE_OP_LESS_OR_EQUAL);

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

	VkPushConstantRange push_constant = {};
	push_constant.offset = 0;
	push_constant.size = sizeof(MeshPushConstants);
	push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayout layouts[] = {
		_CameraSetLayout,
		GPUSetLayout,
		_singleTextureSetLayout};

	VkPipelineLayoutCreateInfo mesh_pipeline_layout_info = {};
	mesh_pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	mesh_pipeline_layout_info.pNext = nullptr;
	mesh_pipeline_layout_info.setLayoutCount = 3;
	mesh_pipeline_layout_info.pSetLayouts = layouts;
	mesh_pipeline_layout_info.pushConstantRangeCount = 1;
	mesh_pipeline_layout_info.pPushConstantRanges = &push_constant;

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
	_triangleMesh._vertices.resize(36);

	_triangleMesh._vertices[0].position = glm::vec3{1.0f, -1.0f, 1.0f};
	_triangleMesh._vertices[1].position = glm::vec3{1.0f, -1.0f, -1.0f};
	_triangleMesh._vertices[2].position = glm::vec3{1.0f, 1.0f, -1.0f};

	_triangleMesh._vertices[3].position = glm::vec3{1.0f, 1.0f, -1.0f};
	_triangleMesh._vertices[4].position = glm::vec3{1.0f, 1.0f, 1.0f};
	_triangleMesh._vertices[5].position = glm::vec3{1.0f, -1.0f, 1.0f};

	_triangleMesh._vertices[6].position = glm::vec3{-1.0f, -1.0f, -1.0f};
	_triangleMesh._vertices[7].position = glm::vec3{-1.0f, -1.0f, 1.0f};
	_triangleMesh._vertices[8].position = glm::vec3{-1.0f, 1.0f, 1.0f};

	_triangleMesh._vertices[9].position = glm::vec3{-1.0f, 1.0f, 1.0f};
	_triangleMesh._vertices[10].position = glm::vec3{-1.0f, 1.0f, -1.0f};
	_triangleMesh._vertices[11].position = glm::vec3{-1.0f, -1.0f, -1.0f};

	_triangleMesh._vertices[12].position = glm::vec3{-1.0f, 1.0f, 1.0f};
	_triangleMesh._vertices[13].position = glm::vec3{1.0f, 1.0f, 1.0f};
	_triangleMesh._vertices[14].position = glm::vec3{1.0f, 1.0f, -1.0f};

	_triangleMesh._vertices[15].position = glm::vec3{1.0f, 1.0f, -1.0f};
	_triangleMesh._vertices[16].position = glm::vec3{-1.0f, 1.0f, -1.0f};
	_triangleMesh._vertices[17].position = glm::vec3{-1.0f, 1.0f, 1.0f};

	_triangleMesh._vertices[18].position = glm::vec3{-1.0f, -1.0f, -1.0f};
	_triangleMesh._vertices[19].position = glm::vec3{1.0f, -1.0f, -1.0f};
	_triangleMesh._vertices[20].position = glm::vec3{1.0f, -1.0f, 1.0f};

	_triangleMesh._vertices[21].position = glm::vec3{1.0f, -1.0f, 1.0f};
	_triangleMesh._vertices[22].position = glm::vec3{-1.0f, -1.0f, 1.0f};
	_triangleMesh._vertices[23].position = glm::vec3{-1.0f, -1.0f, -1.0f};

	_triangleMesh._vertices[24].position = glm::vec3{-1.0f, -1.0f, 1.0f};
	_triangleMesh._vertices[25].position = glm::vec3{1.0f, -1.0f, 1.0f};
	_triangleMesh._vertices[26].position = glm::vec3{1.0f, 1.0f, 1.0f};

	_triangleMesh._vertices[27].position = glm::vec3{1.0f, 1.0f, 1.0f};
	_triangleMesh._vertices[28].position = glm::vec3{-1.0f, 1.0f, 1.0f};
	_triangleMesh._vertices[29].position = glm::vec3{-1.0f, -1.0f, 1.0f};

	_triangleMesh._vertices[30].position = glm::vec3{1.0f, -1.0f, -1.0f};
	_triangleMesh._vertices[31].position = glm::vec3{-1.0f, -1.0f, -1.0f};
	_triangleMesh._vertices[32].position = glm::vec3{-1.0f, 1.0f, -1.0f};

	_triangleMesh._vertices[33].position = glm::vec3{-1.0f, 1.0f, -1.0f};
	_triangleMesh._vertices[34].position = glm::vec3{1.0f, 1.0f, -1.0f};
	_triangleMesh._vertices[35].position = glm::vec3{1.0f, -1.0f, -1.0f};

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

	VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo,
							 &mesh._vertexBuffer._buffer,
							 &mesh._vertexBuffer._allocation,
							 nullptr));

	_mainDeletionQueue.push_function([=]()
									 { vmaDestroyBuffer(_allocator, mesh._vertexBuffer._buffer, mesh._vertexBuffer._allocation); });

	void *data;
	vmaMapMemory(_allocator, mesh._vertexBuffer._allocation, &data);
	memcpy(data, mesh._vertices.data(), mesh._vertices.size() * sizeof(Vertex));
	vmaUnmapMemory(_allocator, mesh._vertexBuffer._allocation);
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

void VulkanEngine::init_descriptors()
{

	const size_t sceneParamBufferSize = FRAME_OVERLAP * pad_uniform_buffer_size(sizeof(GPUSceneData1));
	GPUParameterBuffer = create_buffer(sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	VkDescriptorSetLayoutBinding textureBind = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

	VkDescriptorSetLayoutCreateInfo set3info = {};
	set3info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set3info.bindingCount = 1;
	set3info.flags = 0;
	set3info.pNext = nullptr;
	set3info.pBindings = &textureBind;

	VK_CHECK(vkCreateDescriptorSetLayout(_device, &set3info, nullptr, &_singleTextureSetLayout));

	VkDescriptorSetLayoutBinding GPUBufferBinding = {};
	GPUBufferBinding.binding = 1;
	GPUBufferBinding.descriptorCount = 1;
	GPUBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	GPUBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo gpuSetInfo = {};
	gpuSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	gpuSetInfo.pNext = nullptr;
	gpuSetInfo.bindingCount = 1;
	gpuSetInfo.flags = 0;
	gpuSetInfo.pBindings = &GPUBufferBinding;

	VK_CHECK(vkCreateDescriptorSetLayout(_device, &gpuSetInfo, nullptr, &GPUSetLayout));

	VkDescriptorSetLayoutBinding camBufferBinding = {};
	camBufferBinding.binding = 0;
	camBufferBinding.descriptorCount = 1;
	camBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	camBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo camSetInfo = {};
	camSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	camSetInfo.pNext = nullptr;
	camSetInfo.bindingCount = 1;
	camSetInfo.flags = 0;
	camSetInfo.pBindings = &camBufferBinding;

	VK_CHECK(vkCreateDescriptorSetLayout(_device, &camSetInfo, nullptr, &_CameraSetLayout));

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

	VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &_descriptorPool));

	cameraBuffer = create_buffer(sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	VkDescriptorSetAllocateInfo camAllocInfo = {};
	camAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	camAllocInfo.pNext = nullptr;
	camAllocInfo.descriptorPool = _descriptorPool;
	camAllocInfo.descriptorSetCount = 1;
	camAllocInfo.pSetLayouts = &_CameraSetLayout;

	VK_CHECK(vkAllocateDescriptorSets(_device, &camAllocInfo, &CameraDescriptor));

	VkDescriptorSetAllocateInfo gpuAllocInfo = {};
	gpuAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	gpuAllocInfo.pNext = nullptr;
	gpuAllocInfo.descriptorPool = _descriptorPool;
	gpuAllocInfo.descriptorSetCount = 1;
	gpuAllocInfo.pSetLayouts = &GPUSetLayout;

	VK_CHECK(vkAllocateDescriptorSets(_device, &gpuAllocInfo, &GPUDescriptor));

	VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR);
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	VK_CHECK(vkCreateSampler(_device, &samplerInfo, nullptr, &_blockySampler));

	_mainDeletionQueue.push_function([=]()
									 { vkDestroySampler(_device, _blockySampler, nullptr); });

	std::array<std::string, 6> cubeFaces = {
		"assets/cubemap_6/right.jpg",  // +X
		"assets/cubemap_6/left.jpg",   // -X
		"assets/cubemap_6/top.jpg",	   // +Y
		"assets/cubemap_6/bottom.jpg", // -Y
		"assets/cubemap_6/front.jpg",  // +Z
		"assets/cubemap_6/back.jpg"	   // -Z
	};

	bool texLoaded = vkutil::load_cubemap_from_files(*this, cubeFaces, _texture);

	if (texLoaded)
	{
		VkImageViewCreateInfo imageInfo = vkinit::imageview_create_info(VK_FORMAT_R8G8B8A8_SRGB, _texture.image, VK_IMAGE_ASPECT_COLOR_BIT);
		imageInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		imageInfo.subresourceRange.layerCount = 6;
		VK_CHECK(vkCreateImageView(_device, &imageInfo, nullptr, &_textureImageView));

		_mainDeletionQueue.push_function([=]()
										 { vkDestroyImageView(_device, _textureImageView, nullptr); });
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

		VkWriteDescriptorSet texture1 = {};
		texture1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		texture1.pNext = nullptr;
		texture1.dstBinding = 0;
		texture1.dstSet = TextureDescriptor;
		texture1.descriptorCount = 1;
		texture1.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		texture1.pImageInfo = &imageBufferInfo;

		vkUpdateDescriptorSets(_device, 1, &texture1, 0, nullptr);
	}
	else
	{
		std::cout << "Failed to load texture, set 2 will not be bound in draw()" << std::endl;
		TextureDescriptor = VK_NULL_HANDLE;
	}

	VkDescriptorBufferInfo camBufferInfo = {};
	camBufferInfo.buffer = cameraBuffer._buffer;
	camBufferInfo.offset = 0;
	camBufferInfo.range = sizeof(GPUCameraData);

	VkWriteDescriptorSet camWrite = {};
	camWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	camWrite.pNext = nullptr;
	camWrite.dstBinding = 0;
	camWrite.dstSet = CameraDescriptor;
	camWrite.descriptorCount = 1;
	camWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	camWrite.pBufferInfo = &camBufferInfo;

	VkDescriptorBufferInfo sceneBufferInfo = {};
	sceneBufferInfo.buffer = GPUParameterBuffer._buffer;
	sceneBufferInfo.offset = 0;
	sceneBufferInfo.range = sizeof(GPUSceneData1);

	VkWriteDescriptorSet sceneWrite = {};
	sceneWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	sceneWrite.pNext = nullptr;
	sceneWrite.dstBinding = 1;
	sceneWrite.dstSet = GPUDescriptor;
	sceneWrite.descriptorCount = 1;
	sceneWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	sceneWrite.pBufferInfo = &sceneBufferInfo;

	VkWriteDescriptorSet setWrites[] = {camWrite, sceneWrite};
	vkUpdateDescriptorSets(_device, 2, setWrites, 0, nullptr);

	_mainDeletionQueue.push_function([=]()
									 {
		vmaDestroyBuffer(_allocator, GPUParameterBuffer._buffer, GPUParameterBuffer._allocation);
		vmaDestroyBuffer(_allocator, cameraBuffer._buffer, cameraBuffer._allocation);

		vkDestroyDescriptorSetLayout(_device, GPUSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(_device, _CameraSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(_device, _singleTextureSetLayout, nullptr);
		vkDestroyDescriptorPool(_device, _descriptorPool, nullptr); });
}