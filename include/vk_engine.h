

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
#include "FBX_Anim.h"

#include <stb_image_write.h>

class VulkanEngine
{
public:
	bool _isInitialized{false};
	int _frameNumber{0};

	VkExtent2D _windowExtent{};

	struct SDL_Window *_window{nullptr};

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
	VkRenderPass _OffScreen_renderPass;

	VkSurfaceKHR _surface;
	VkSwapchainKHR _swapchain;
	VkFormat _swachainImageFormat;

	std::vector<VkFramebuffer> _framebuffers;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;

	VkPipeline _trianglePipeline;

	DeletionQueue _mainDeletionQueue;

	VmaAllocator _allocator;

	VkPipeline _meshPipeline;

	std::vector<Mesh> _ModelMeshes;

	void load_meshes();
	void upload_mesh(Mesh &mesh);

	void load_Quadmesh();
	Mesh QuadMesh;
	VkPipeline _QuadPipeline;
	VkPipelineLayout _QuadPipelineLayout;
	AllocatedImage _OffScreentexture;

	AllocatedImage _offscreenImage;
	VkImageView _offscreenImageView;
	VkFramebuffer _offscreenFramebuffer;
	VkFormat _offscreenFormat = VK_FORMAT_R8G8B8A8_UNORM;
	VkSampler _offscreenSampler;

	AllocatedImage _offscreenDepthImage;
	VkImageView _offscreenDepthImageView;

	void init_offscreen();

	void upload_mesh(std::vector<Mesh> &mesh);

	void init();

	void cleanup();

	void draw();

	void run();

	bool load_shader_module(const char *filePath, VkShaderModule *outShaderModule);

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

	Texture_Slots Offtxs;

	VkImageView _depthImageView;
	AllocatedImage _depthImage;
	VkFormat _depthFormat;


	FBX_ANIM::FBX_ANIMATION fbx_anim ;
	FBX_Model_Loader fbxl ;



	std::unordered_map<ufbx_node *, std::map<double, ufbx_transform>>::const_iterator iterator;
	std::set<double>::const_iterator time_range_itr;

private:
	void init_vulkan();

	void init_swapchain();

	void init_default_renderpass();

	void init_framebuffers();

	void init_commands();

	void init_sync_structures();
};