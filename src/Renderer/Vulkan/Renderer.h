#pragma once
#include "Wrappers/Window.h"
#include "Wrappers/Context.h"
#include "Wrappers/Swapchain.h"
#include "Wrappers/Pipeline.h"

class Renderer
{
public:
	bool initialiseRenderer();

	Renderer& addPipeline(Pipeline p, std::initializer_list<Shader*> shaders);
	Renderer& addPipeline(Pipeline p, PipelineSettings s, std::initializer_list<Shader*> shaders);

	Renderer& buildPipelines();

	~Renderer();
private:

	Window		window; // GLFWwindow, Surface
	Context		context; // VkDevice, VkInstance, VkPhysicalDevice
	Swapchain	swapchain; // VkSwapchainKHR
	std::vector<Pipeline> pipelines;
	std::vector<PipelineSettings> pipelineSettings;
	std::vector<std::initializer_list<Shader*>> shaders;
};
