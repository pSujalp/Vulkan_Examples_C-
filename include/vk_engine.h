// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
#include <vector>
#include "PipelineBuilder.h"
#include <vk_mesh.h>
#include <glm/gtx/transform.hpp>
#include "camera.h"


#include "vk_textures.h"
#include "Texture_Slots.h"
#include "ufbx.h"
#include "FBX_Model_Loader.h"


#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
#include <stdio.h>
#include <SDL.h>


class VulkanEngine {
public:

	bool _isInitialized{ false };
	int _frameNumber {0};

	VkExtent2D _windowExtent{  };

	struct SDL_Window* _window{ nullptr };

	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debug_messenger;
	VkPhysicalDevice _chosenGPU;
	VkDevice _device;

	VkSemaphore _presentSemaphore, _renderSemaphore;
	VkFence _renderFence;

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;
	
	VkRenderPass _renderPass;

	VkSurfaceKHR _surface;
	VkSwapchainKHR _swapchain;
	VkFormat _swachainImageFormat;

	std::vector<VkFramebuffer> _framebuffers;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;


	VkPipeline _trianglePipeline;
	VkPipelineCache pipelineCache;

	DeletionQueue _mainDeletionQueue;

	VmaAllocator _allocator;


	VkPipeline _meshPipeline;


	std::vector<Mesh> _ModelMeshes;


	void load_meshes();

	void upload_mesh(std::vector<Mesh>& mesh);

	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();

	bool load_shader_module(const char* filePath, VkShaderModule* outShaderModule);

	void init_pipelines();

	VkPipelineLayout _trianglePipelineLayout;

	VkPipelineLayout _meshPipelineLayout;
	AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

	void init_descriptor();

	VkDescriptorPool _descriptorPool;

	VkDescriptorSetLayout _singleTextureSetLayout;
	VkDescriptorSet TextureDescriptor = VK_NULL_HANDLE;
	VkSampler _blockySampler;
	AllocatedImage _texture;
	VkImageView _textureImageView;
	

	VkDescriptorSetLayout _mixsingleTextureSetLayout;
	VkDescriptorSet TextureDescriptor1 = VK_NULL_HANDLE;
	AllocatedImage _texture1;
	VkImageView _textureImageView1;

	
	Camera camera;
	Texture_Slots txs;

    VkImageView _depthImageView;
	AllocatedImage _depthImage;
	VkFormat _depthFormat;

	
private:

	void init_vulkan();

	void init_swapchain();

	void init_default_renderpass();

	void init_framebuffers();

	void init_commands();

	void init_sync_structures();
};