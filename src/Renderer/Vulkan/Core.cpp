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

	const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0
	};

	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

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

		//auto frameInfo = swapchain.BeginFrame(buffers[swapchain.getIndex()]);


		//static auto startTime = std::chrono::high_resolution_clock::now();

		//auto currentTime = std::chrono::high_resolution_clock::now();
		//float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		//UniformBufferObject ubo{};
		//ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		//ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		//ubo.proj = glm::perspective(glm::radians(45.0f), swapchain.getExtent().width / (float)swapchain.getExtent().height, 0.1f, 10.0f);
		//ubo.proj[1][1] *= -1;
		//
		//descriptorCache.setResource(descKey, "ubo", frameInfo.offset, &ubo, sizeof(UniformBufferObject));

		//auto result = swapchain.EndFrame(frameInfo, device.queues.graphics);
		//if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		//	windowResize();

		return true;
	}

	void Core::windowResize()
	{

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
