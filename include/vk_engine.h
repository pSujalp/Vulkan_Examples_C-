// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
#include <vector>
#include "DeletionQueue.h" 
#include "vk_mem_alloc.h"



//> framedata
struct FrameData {
	VkSemaphore _swapchainSemaphore, _renderSemaphore;
	VkFence _renderFence;

	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;

	DeletionQueue _deletionQueue;
};




constexpr unsigned int FRAME_OVERLAP = 2;
//< framedata

class VulkanEngine {
public:

	bool _isInitialized{ false };
	int _frameNumber {0};

	VkExtent2D _windowExtent{ 1700 , 900 };

	struct SDL_Window* _window{ nullptr };

//> inst_init
	VkInstance _instance;// Vulkan library handle
	VkDebugUtilsMessengerEXT _debug_messenger;// Vulkan debug output handle
	VkPhysicalDevice _chosenGPU;// GPU chosen as the default device
	VkDevice _device; // Vulkan device for commands
	VkSurfaceKHR _surface;// Vulkan window surface

	VmaAllocator _allocator;
//< inst_init

//> queues
	FrameData _frames[FRAME_OVERLAP];

	FrameData& get_current_frame()  { return _frames[_frameNumber % FRAME_OVERLAP]; };

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;
//< queues
	
//> swap_init
	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat;

	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;
	VkExtent2D _swapchainExtent;

	DeletionQueue _mainDeletionQueue;

	AllocatedImage _drawImage;
	VkExtent2D _drawExtent;
//< swap_init

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();

	bool stop_rendering{false};
private:

	bool init_vulkan();

	void init_swapchain();

	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();

	void init_commands();

	void init_sync_structures();

	void draw_background(VkCommandBuffer cmd);


};