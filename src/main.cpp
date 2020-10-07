#define VMA_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS


#include "Renderer/Vulkan/Core.h"
#include "imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include "GUI.h"
#include "Sphere.h"
#include "Camera.h"
#include "Renderer/Vulkan/Memory/Allocator.h"

using namespace Renderer;

struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

int main()
{
	auto renderer = std::make_unique<Core>(1200, 800, "Window");
	GUI gui;

	renderer->Initialise();
	gui.initialise(renderer.get());

	auto program = renderer->GetShaderManager()->getProgram({ renderer->GetShaderManager()->defaultVertex(), renderer->GetShaderManager()->defaultFragment() });
	program->initialiseResources(renderer->GetDevice()->GetDevice());

	DescriptorSetKey descriptorSetKey = { program };
	const auto vert = Vertex::defaultVertex();

	Camera cam = Camera{ 0, 1200, 0, 800 };
	cam.SetModel(rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
	cam.SetView(lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
	cam.SetProj(glm::perspective(glm::radians(45.0f), 1200 / 800.0f, 0.1f, 10.0f));

	Sphere* sp = new Sphere(renderer->GetAllocator(), 50000);
	UniformBufferObject ubo = {};


	int points = 0;
	bool fibo = false;

	renderer->GetRendergraph()->AddPass(PassDesc().SetName("GPU Drawing Triangle").SetInitialisationFunc([&descriptorSetKey](Tether& tether) { tether.GetDescriptorCache()->WriteBuffer(descriptorSetKey, "ubo"); }).SetRecordFunc(
		[&renderer, &descriptorSetKey, &vert, &sp, &cam, &ubo, &points, &fibo](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context)
		{
			{
				static auto startTime = std::chrono::high_resolution_clock::now();

				auto currentTime = std::chrono::high_resolution_clock::now();
				float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

				cam.SetModel(rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f)));

				ubo.model = cam.GetModel();
				ubo.view = cam.GetView();
				ubo.proj = cam.GetProj();

				renderer->GetDescriptorSetCache()->SetResource(descriptorSetKey, "ubo", &ubo, sizeof(UniformBufferObject));
			}

			{
				sp->ReCalculate( points = (points + 1 > 100000) ? 100000 : points + 1);

				if (ImGui::SliderInt("Points", &points, 1000, 100000)) sp->ReCalculate(points);
				if (ImGui::Checkbox("Use Fibonacci", &fibo)) sp->SetFibo(fibo);
			}
			
			renderer->GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.getDefaultRenderpass(), context.GetExtent(), vert, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
				descriptorSetKey.program);

			renderer->GetDescriptorSetCache()->BindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorSetKey);

			sp->Draw(buffer);

			renderer->GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.getDefaultRenderpass(), context.GetExtent(), vert, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
				descriptorSetKey.program);

			renderer->GetDescriptorSetCache()->BindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorSetKey);
			
			sp->DrawSweepline(buffer);
		}));


	gui.AddToGraph(renderer->GetRendergraph());

	renderer->GetRendergraph()->Initialise();

	while (renderer->Run()) { renderer->GetRendergraph()->Execute(); }

	vkDeviceWaitIdle(*renderer->GetDevice());

	sp->Cleanup();
	delete sp;
	delete program;
	delete gui.key.program;
}
