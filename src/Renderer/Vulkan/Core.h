#pragma once
#include "Wrappers/Window.h"
#include "Wrappers/Context.h"
#include "Wrappers/Swapchain.h"
#include "Wrappers/Pipeline.h"

namespace Renderer {

	/*
		SwapchainSync (Default) - One copy of the buffer per image in the swapchain
		SingleBuffered - Only one copy of each type of buffer, this should only be used when there are large vertex buffers which are not updated often/quickly
		DoubleBuffered - Two copies of each type of buffer, the optimal scenario, while one buffer is submitted and drawn on, there is an alternate buffer being filled by CPU
		TripleBuffered - Three copies of each type of buffer, used when the CPU can outperform the GPU - Will potentially noticable input lag
	*/
	enum class RendererBufferSettings { SwapchainSync, SingleBuffered, DoubleBuffered, TripleBuffered, };

	struct RendererSettings
	{
		RendererBufferSettings buffering = RendererBufferSettings::SwapchainSync;
	};


	class Core
	{
	public:
		bool initialiseRenderer();

		~Core();
	private:

		Window		window; // GLFWwindow, Surface
		Context		context; // VkDevice, VkInstance, VkPhysicalDevice
		Swapchain	swapchain; // VkSwapchainKHR
		std::vector<Pipeline> pipelines;
		std::vector<PipelineSettings> pipelineSettings;
		std::vector<std::initializer_list<Shader*>> shaders;


		/*

		Runtime functionality Renderer

		*/
	public:





	private:
		RendererSettings settings;
		uint32_t maxFramesInFlight;
		uint32_t bufferCopies;
	};

}