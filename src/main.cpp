#include <vk_engine.h>

int main(void)
{
	VulkanEngine engine;

	engine.init();	
	
	if (engine._isInitialized) {
		engine.run();


		
	}	

	engine.cleanup();	

	return 0;
}