#include <vk_engine.h>

int main(void)
{
	VulkanEngine engine;

	engine.init();	
	
	engine.run();	

	engine.cleanup();	

	return 0;
}