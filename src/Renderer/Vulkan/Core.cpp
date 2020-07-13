#include "../../Utils/Logging.h"
#include "Core.h"
#include "glslang/Public/ShaderLang.h"
#include <stdexcept>

namespace Renderer
{
	Core::Core(int x, int y, const char* name)
	{
		TempLogger::Init();
		VerboseLog("Initialising Window");
		window.initialiseWindow(x, y, name);
	}

	bool Core::Initialise()
	{
		/*
			This will initialise the (mostly) static states of the renderer, customisation can come later.
		*/
		bool success = false;

		VerboseLog("Building Window");
		window.buildWindow();
		VerboseLog("Building Instance");
		Wrappers::buildInstance(&context);
		VerboseLog("Building Surface");
		success = window.buildSurface(&context);
		Wrappers::pickPhysicalDevice(&context, &window);
		VerboseLog("Building Device");		
		Wrappers::buildDevice(&context);
		VerboseLog("Initialising VMA Allocator");
		initialiseAllocator();
		VerboseLog("Building Swapchain");
		success = swapchain.buildSwapchain(&context, &window);
		VerboseLog("Building renderpass Cache");
		renderpassCache.buildCache(&context.device.device);
		RenderpassKey key = RenderpassKey(
			{ { swapchain.getFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR } }
		, {});
		VerboseLog("Adding to renderpass Cache");
		success = renderpassCache.add(key);
		VerboseLog("Building pipeline Cache");
		pipelineCache.buildCache(&context.device.device);

		VerboseLog("Baking pipeline Key");
		auto gpKey = pipelineCache.bakeKey(
			renderpassCache[key]->getHandle(),
			swapchain.extent,
			DepthSettings::Disabled(),
			{ BlendSettings::Add() },
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			{
				std::make_shared<Shader>(ShaderType::Vertex, "resources/VertexShader.vert"),
				std::make_shared<Shader>(ShaderType::Fragment, "resources/FragmentShader.frag")
			});
		VerboseLog("Inserting pipeline Key");
		auto dump = pipelineCache[gpKey];

		Assert(success, "Failed to build state for the renderer");

		VerboseLog("Creating framebuffer Key");
		auto fKey = FramebufferKey(swapchain.getImageViews(), *renderpassCache[key], swapchain.getExtent());
		VerboseLog("Building framebuffer cache");
		framebufferCache.buildCache(&context.device.device);
		auto frame = framebufferCache.get(fKey);



		maxFramesInFlight = swapchain.getSize();

		switch (settings.buffering) {
			case RendererBufferSettings::SwapchainSync: bufferCopies = maxFramesInFlight;
			case RendererBufferSettings::SingleBuffered: bufferCopies = 1;
			case RendererBufferSettings::DoubleBuffered: bufferCopies = 2;
			case RendererBufferSettings::TripleBuffered: bufferCopies = 3;
		}

		return true;
	}

	void Core::initialiseAllocator()
	{
		VmaAllocatorCreateInfo createInfo = {};
		createInfo.device = context.getDevice();
		createInfo.physicalDevice = context.getPhysicalDevice();
		createInfo.instance = context.getInstance();
		createInfo.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;

		vmaCreateAllocator(&createInfo, &allocator);
	}

	Core::~Core()
	{
		framebufferCache.clearCache();
		pipelineCache.clearCache();
		renderpassCache.clearCache();
		swapchain.cleanupSwapchain();
		vmaDestroyAllocator(allocator);
		context.cleanupDevice();
		window.cleanup();
		context.cleanupInstance();
	}
	
}