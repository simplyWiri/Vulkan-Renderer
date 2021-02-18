#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS

#include "Renderer/Core.h"
#include "imgui.h"
#include <glm/gtc/matrix_transform.hpp>
#include "GUI.h"
#include "Utils/Camera.h"
#include "World/Generation/PlanetGenerator.h"
#include "World/PlanetRenderer.h"
#include "Tracy.hpp"
#include <Utils\AnchoredCamera.h>

using namespace Renderer;
using namespace World::Generation;

void* operator new(std :: size_t count)
{
	auto ptr = malloc(count);
	TracyAlloc (ptr , count);
	return ptr;
}
void operator delete(void* ptr) noexcept
{
	TracyFree(ptr);
	free(ptr);
}

struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

int main()
{
	Settings s = {};
	s.width = 1200;
	s.height = 840;

	auto renderer = std::make_unique<Core>(s);
	GUI gui;
	
	{
		ZoneScopedNC("Initialise Renderer", tracy::Color::Red)
		renderer->Initialise();
	}
	
	gui.initialise(renderer.get());

	auto program = renderer->GetShaderManager()->getProgram({ renderer->GetShaderManager()->defaultVertex(), renderer->GetShaderManager()->defaultFragment() });
	program->InitialiseResources(renderer->GetDevice()->GetDevice());

	DescriptorSetKey descriptorSetKey = { program };
	const auto vert = Vertex::defaultVertex();

	float fov = 45.0f;
	
	auto anchoredCam = AnchoredCamera( {0, 0, 0}, {0, 1, 0}, { 0, 0, 3});

	anchoredCam.SetModel(rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
	anchoredCam.SetProj(glm::perspective(glm::radians(fov), s.width / static_cast<float>(s.height), 0.1f, 10.0f));

	auto* gen = new PlanetGenerator();
	auto planetRenderer = new World::PlanetRenderer(gen->RetrievePlanet(), renderer->GetAllocator());
	UniformBufferObject ubo = {};


	float lastX = s.width/2, lastY = s.height/2;
	bool active = false;
	float camSpeed = 2.5f;

	auto mouseMoveCallback = [&](float xpos, float ypos)
	{
	    float xOffset = lastX - xpos;
	    float yOffset = ypos - lastY; // y flipped in vulkan 
	    lastX = xpos;
	    lastY = ypos;

		if(!active) return false;

	    xOffset *= camSpeed;
	    yOffset *= camSpeed;

		float zoomMultiplier = glm::clamp(fov / 45.0f, 0.25f, 1.0f);
		
		anchoredCam.Rotate({ xOffset * zoomMultiplier, yOffset * zoomMultiplier});

		return false;	
	};

	auto mouseClickCallback = [&active](int button, int action)
	{
		if(button != 0) return false;

		if(action == GLFW_RELEASE) active = false;
		else active = true;

		return false;
	};

	auto scrollCallback = [&](float yOffset)
	{
		fov -= yOffset;

		fov = glm::clamp(fov, 1.0f, 45.0f);

		anchoredCam.SetProj(glm::perspective(glm::radians(fov), static_cast<float>(s.width) / static_cast<float>(s.height), 0.1f, 10.0f));

		return false;
	};

	renderer->GetInputHandler()->RegisterMouseClickCallBack(mouseClickCallback, InputPriority::LOW_PRIORITY);
	renderer->GetInputHandler()->RegisterMouseMoveCallBack(mouseMoveCallback, InputPriority::LOW_PRIORITY);
	renderer->GetInputHandler()->RegisterMouseScrollCallBack(scrollCallback, InputPriority::LOW_PRIORITY);
	
	renderer->GetRendergraph()->AddPass(PassDesc()
		.SetName("GPU Drawing Triangle")
		.SetInitialisationFunc([&descriptorSetKey](Tether& tether) { tether.GetDescriptorCache()->WriteBuffer(descriptorSetKey, "ubo"); })
		.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context)
		{
			{
				ZoneScopedN("Update Camera and Set Descriptor Cache")
				static auto startTime = std::chrono::high_resolution_clock::now();

				auto currentTime = std::chrono::high_resolution_clock::now();
				float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
				
				camSpeed = 2.5f * frameInfo.delta;

			    if (glfwGetKey(context.window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(context.window, true);

				ubo.model = anchoredCam.GetModel();
				ubo.view = anchoredCam.GetView(); 
				ubo.proj = anchoredCam.GetProj();

				context.GetDescriptorSetCache()->SetResource(descriptorSetKey, "ubo", &ubo, sizeof(UniformBufferObject));
			}

			context.GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetDefaultRenderpass(), context.GetSwapchainExtent(), vert, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_POINT_LIST,descriptorSetKey.program);

			context.GetDescriptorSetCache()->BindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorSetKey);

			planetRenderer->DrawCells(buffer);
			planetRenderer->DrawVertices(buffer);

			gen->Step(5);
		}));


	gui.AddToGraph(renderer->GetRendergraph());

	renderer->GetRendergraph()->Initialise();

	while (renderer->Run()) { renderer->GetRendergraph()->Execute(); }

	vkDeviceWaitIdle(*renderer->GetDevice());

	delete gen;
	delete planetRenderer;
	delete program;
	delete gui.key.program;
}
