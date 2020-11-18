#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS


#include "Renderer/Core.h"
#include "imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include "Utils/GUI.h"
#include "Sphere.h"
#include "Utils/Camera.h"

using namespace Renderer;

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

	renderer->Initialise();
	gui.initialise(renderer.get());

	auto program = renderer->GetShaderManager()->getProgram({ renderer->GetShaderManager()->defaultVertex(), renderer->GetShaderManager()->defaultFragment() });
	program->InitialiseResources(renderer->GetDevice()->GetDevice());

	DescriptorSetKey descriptorSetKey = { program };
	const auto vert = Vertex::defaultVertex();

	auto cam = Camera{ 0, 1200, 0, 800 };
	cam.SetModel(rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
	cam.SetView(lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
	cam.SetProj(glm::perspective(glm::radians(45.0f), 1200 / 800.0f, 0.1f, 10.0f));

	auto* sp = new Sphere(renderer->GetAllocator(), 50000);
	UniformBufferObject ubo = {};

	int points = 0;
	bool fibo = false;
	bool lock = false;

	renderer->GetRenderGraph()->AddPass(RenderGraphBuilder()
		.SetName("GPU Drawing Triangle")
		.SetInitialisationFunc([&descriptorSetKey](Tether& tether) { tether.GetDescriptorCache()->WriteBuffer(descriptorSetKey, "ubo"); })
		.SetRecordFunc([&descriptorSetKey, &vert, &sp, &cam, &ubo, &points, &fibo, &lock](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context)
		{
			{
				static auto startTime = std::chrono::high_resolution_clock::now();

				auto currentTime = std::chrono::high_resolution_clock::now();
				float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

				cam.SetModel(rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f)));

				ubo.model = cam.GetModel();
				ubo.view = cam.GetView();
				ubo.proj = cam.GetProj();

				context.GetDescriptorSetCache()->SetResource(descriptorSetKey, "ubo", &ubo, sizeof(UniformBufferObject));
			}

			{
				ImGui::Begin("Options");
				
				if (!lock && points < 50000) { sp->ReCalculate(points = (points + 1 > 50000) ? 50000 : points + 1); }
				if (ImGui::SliderInt("Points", &points, 1000, 50000)) sp->ReCalculate(points);
				if (ImGui::Checkbox("Use Fibonacci", &fibo)) sp->SetFibo(fibo);
				ImGui::Checkbox("Lock Recalcs", &lock);

				ImGui::End();
			}

			context.GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetDefaultRenderpass(), context.GetSwapchainExtent(), vert, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
				descriptorSetKey.program);

			context.GetDescriptorSetCache()->BindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorSetKey);

			sp->Draw(buffer);

			context.GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetDefaultRenderpass(), context.GetSwapchainExtent(), vert, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
				descriptorSetKey.program);

			context.GetDescriptorSetCache()->BindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorSetKey);

			sp->DrawSweepline(buffer);
		}));


	gui.AddToGraph(renderer->GetRenderGraph());

	renderer->GetRenderGraph()->Initialise();

	while (renderer->Run()) { renderer->GetRenderGraph()->Execute(); }

	vkDeviceWaitIdle(*renderer->GetDevice());

	sp->Cleanup();
	delete sp;
	delete program;
}
