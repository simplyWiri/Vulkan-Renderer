#include "Core.h"
#include "glslang/Public/ShaderLang.h"
#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>

namespace Renderer
{
	Core::Core(int x, int y, const char* name)
	{
		TempLogger::Init();
	}

	bool Core::Initialise()
	{
		/*
			This will initialise the (mostly) static states of the renderer, customisation can come later.
		*/
		bool success = false;
		swapchain.Initialise(device.getDevice(), device.getInstance(), device.getPhysicalDevice());

		swapchain.BuildWindow(640, 400, "Vulk");

		device.BuildInstance(true);
		swapchain.BuildSurface();
		device.PickPhysicalDevice(swapchain.getSurface());
		device.BuildLogicalDevice(swapchain.getPresentQueue());
		device.BuildAllocator();

		swapchain.BuildSwapchain();
		swapchain.InitialiseSyncObjects();

		rendergraph = std::make_unique<Rendergraph>(this);

		return true;
	}

	bool Core::Run()
	{
		if (glfwWindowShouldClose(swapchain.getWindow())) return false;

		glfwPollEvents();

		return true;
	}

	void Core::windowResize()
	{
		int width = 0, height = 0;
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(swapchain.getWindow(), &width, &height);
			glfwWaitEvents();
		}
		vkDeviceWaitIdle(device);
		vkQueueWaitIdle(device.queues.graphics);
		
		swapchain.BuildSwapchain();
		rendergraph->Rebuild();
	}

	void Core::BeginFrame(VkCommandBuffer& buffer, FrameInfo& info)
	{
		info = swapchain.BeginFrame(buffer);
	}

	void Core::EndFrame(FrameInfo info)
	{
		auto result = swapchain.EndFrame(info, device.queues.graphics);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			windowResize();
	}

	Core::~Core()
	{
		//vertexBuffer->~Buffer();
		//indexBuffer->~Buffer();

		//vkDestroyCommandPool(device, commandPool, nullptr);
		//framebufferCache.clearCache();
		//pipelineCache.clearCache();
		//renderpassCache.clearCache();
		//descriptorCache.clearCache();
	}
}
