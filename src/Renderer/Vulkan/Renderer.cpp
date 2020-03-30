#include "Renderer.h"

bool Renderer::initialiseRenderer()
{
	window.initialiseWindow(640, 400, "Vulkan Renderer");

	Wrappers::buildWindow(&window);
	Wrappers::buildInstance(&context);
	Wrappers::buildSurface(&window, &context);
	Wrappers::pickPhysicalDevice(&context, &window);
	Wrappers::buildDevice(&context);
	return false;
}

Renderer::~Renderer()
{
	context.cleanup();
	window.cleanup();
}