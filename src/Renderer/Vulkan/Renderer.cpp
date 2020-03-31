#include "Renderer.h"

bool Renderer::initialiseRenderer()
{
	/* 
		This will initialise the (mostly) static states of the renderer, customisation can come later.
	*/
	window.initialiseWindow(640, 400, "Vulkan Renderer");

	Wrappers::buildWindow(&window);
	Wrappers::buildInstance(&context);
	Wrappers::buildSurface(&window, &context);
	Wrappers::pickPhysicalDevice(&context, &window);
	Wrappers::buildDevice(&context);
	Wrappers::buildSwapchain(&swapchain, &context, &window);
	return true;
}

Renderer::~Renderer()
{
	swapchain.cleanupSwapchain();
	context.cleanupDevice();
	window.cleanup();
	context.cleanupInstance();
}