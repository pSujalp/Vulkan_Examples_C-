// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
#include <vector>
#include "PipelineBuilder.h"
#include <vk_mesh.h>
#include <glm/gtx/transform.hpp>

#include <unordered_map>

#include "RenderObject.h"
#include "FrameData.h"


constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanEngine
{
public:
	bool _isInitialized{false};
	int _frameNumber{0};

	VkExtent2D _windowExtent{800, 600};

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

	VkSurfaceKHR _surface;
	VkSwapchainKHR _swapchain;
	VkFormat _swachainImageFormat;

	std::vector<VkFramebuffer> _framebuffers;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;

	VkPipeline _trianglePipeline;

	DeletionQueue _mainDeletionQueue;

	VmaAllocator _allocator;

	VkImageView _depthImageView;
	AllocatedImage _depthImage;
	VkFormat _depthFormat;

	VkPipeline _meshPipeline;
	Mesh _triangleMesh;

	Mesh _monkeyMesh;

	FrameData _frames[FRAME_OVERLAP];

    FrameData& get_current_frame();

	void load_meshes();

	void upload_mesh(Mesh &mesh);

	void init();

	// shuts down the engine
	void cleanup();

	// draw loop
	void draw();

	// run main loop
	void run();

	bool load_shader_module(const char *filePath, VkShaderModule *outShaderModule);

	void init_pipelines();

	VkPipelineLayout _trianglePipelineLayout;

	VkPipelineLayout _meshPipelineLayout;

	std::vector<RenderObject> _renderables;

	std::unordered_map<std::string, Material> _materials;
	std::unordered_map<std::string, Mesh> _meshes;
	// functions

	// create material and add it to the map
	Material *create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string &name);

	// returns nullptr if it can't be found
	Material *get_material(const std::string &name);

	// returns nullptr if it can't be found
	Mesh *get_mesh(const std::string &name);

	// our draw function
	void draw_objects(VkCommandBuffer cmd, RenderObject *first, int count);

private:
	void init_vulkan();

	void init_swapchain();

	void init_default_renderpass();

	void init_framebuffers();

	void init_commands();

	void init_sync_structures();

	void init_scene();
};