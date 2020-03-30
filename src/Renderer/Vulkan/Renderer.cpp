#include "Renderer.h"

bool Renderer::initialiseRenderer()
{
	window.initialiseWindow(640, 400, "Vulkan Renderer");

	Wrappers::buildWindow(&window);
	Wrappers::buildInstance(&context);
	Wrappers::buildSurface(&window, &context);
	Wrappers::pickPhysicalDevice(&context, &window);
	Wrappers::buildDevice(&context);
	Wrappers::buildSwapchain(&swapchain, &context, &window);
	return false;
}

Renderer::~Renderer()
{
	swapchain.cleanupSwapchain();
	context.cleanupDevice();
	window.cleanup();
	context.cleanupInstance();
}