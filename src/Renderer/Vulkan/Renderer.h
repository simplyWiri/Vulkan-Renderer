#pragma once
#include "Wrappers/Window.h"
#include "Wrappers/Context.h"
#include "Wrappers/Swapchain.h"

class Renderer
{
public:
	bool initialiseRenderer();

	~Renderer();
private:

	Window		window; // GLFWwindow, Surface
	Context		context; // VkDevice, VkInstance, VkPhysicalDevice
	Swapchain	swapchain; // VkSwapchainKHR
};
