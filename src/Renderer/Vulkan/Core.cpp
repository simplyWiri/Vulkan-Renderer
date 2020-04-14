#include "Core.h"
#include "glslang/Public/ShaderLang.h"
#include <stdexcept>

namespace Renderer {
	bool Core::initialiseRenderer()
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

		maxFramesInFlight = swapchain.getSize();

		switch (settings.buffering) {
		case RendererBufferSettings::SwapchainSync:
			bufferCopies = maxFramesInFlight;
		case RendererBufferSettings::SingleBuffered:
			bufferCopies = 1;
		case RendererBufferSettings::DoubleBuffered:
			bufferCopies = 2;
		case RendererBufferSettings::TripleBuffered:
			bufferCopies = 3;
		}

		return true;
	}

	Core::~Core()
	{
		swapchain.cleanupSwapchain();
		context.cleanupDevice();
		window.cleanup();
		context.cleanupInstance();
	}
}