#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS


#include "Renderer/Core.h"
#include "Utils/GUI.h"
#include "Renderer/RenderGraph/Tether.h"
#include "Renderer/RenderGraph/GraphContext.h"

using namespace Renderer;

struct Information
{
	float time;
	float xMult = 1;
	float yMult = 1;
	float padding;
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

	auto program = renderer->GetShaderManager()->getProgram({ renderer->GetShaderManager()->fullScreenTri(), renderer->GetShaderManager()->get(ShaderType::Fragment, "resources/QuasiCrystals.frag") });
	program->InitialiseResources(renderer->GetDevice()->GetDevice());

	DescriptorSetKey descriptorSetKey = { program };

	auto info = Information{};
	float timeMult = 1;

	renderer->GetRendergraph()->AddPass("Quasi-Crystals", RenderGraphQueue::Graphics)
		.SetInitialisationFunc([](Tether& tether)
		{			
			tether.AddWriteDependencyImage(tether.graph->GetBackBuffer(), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT);
		})
		.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context)
		{
			{
				ImGui::Begin("Options");
				
				ImGui::SliderFloat("Time Multiplier", &timeMult, 1, 15);
				ImGui::SliderFloat("X Scale", &info.xMult, 1, 25);
				ImGui::SliderFloat("Y Scale", &info.yMult, 1, 25);
				info.yMult = info.xMult;

				ImGui::End();
			}

			static auto startTime = std::chrono::high_resolution_clock::now();
			const auto currentTime = std::chrono::high_resolution_clock::now();
			info.time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count() * timeMult;
			
			context.GetDescriptorSetCache()->SetResource(descriptorSetKey, "info", &info, sizeof(Information));

			context.GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetDefaultRenderpass(), context.GetSwapchainExtent(), VertexAttributes{}, DepthSettings::Disabled(), { BlendSettings::Mixed() }, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				descriptorSetKey.program);

			context.GetDescriptorSetCache()->BindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorSetKey);

			vkCmdDraw(buffer, 3, 1, 0, 0);
		});


	gui.AddToGraph(renderer->GetRendergraph());

	renderer->GetRendergraph()->Initialise();

	while (renderer->Run()) { renderer->GetRendergraph()->Execute(); }

	vkDeviceWaitIdle(*renderer->GetDevice());

	delete program;
}
