#include "Renderer.h"
#include "glslang/Public/ShaderLang.h"
#include <stdexcept>
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

Renderer& Renderer::addPipeline(Pipeline p, std::initializer_list<Shader*> shaders)
{
	pipelines.push_back(p);
	pipelineSettings.push_back(PipelineSettings());
	this->shaders.push_back(shaders);
	return *this;
}

Renderer& Renderer::addPipeline(Pipeline p, PipelineSettings s, std::initializer_list<Shader*> shaders)
{
	pipelines.push_back(p);
	pipelineSettings.push_back(s);
	this->shaders.push_back(shaders);
	return *this;
}

Renderer& Renderer::buildPipelines()
{
	glslang::InitializeProcess();

	for (uint32_t i = 0; i < pipelines.size(); i++) {
		pipelines[i].setContext(&context);
		bool compute = false;

		// If any of the shaders are compute, its a compute shader, cannot mix compute & any other type ( i think )
		for (const auto& shader : shaders[i]) {
			if (shader->getType() == ShaderType::Compute)
				compute = true;
			else if (compute)
				// this should be an error
				throw std::runtime_error("Cannot use compute shaders simultaneously with other types of shaders");
		}

		if (compute)
			Wrappers::buildComputePipeline(pipelines[i], pipelineSettings[i], shaders[i]);
		else
			Wrappers::buildGraphicsPipeline(pipelines[i], pipelineSettings[i], shaders[i]);
	}

	glslang::FinalizeProcess();
	return *this;
}

Renderer::~Renderer()
{
	swapchain.cleanupSwapchain();
	context.cleanupDevice();
	window.cleanup();
	context.cleanupInstance();
}