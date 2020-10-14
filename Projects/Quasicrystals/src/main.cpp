#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS


#include "Renderer/Core.h"
#include <glm/gtc/matrix_transform.hpp>
#include "GUI.h"

using namespace Renderer;

struct Information
{
	float time;
	float xMult;
	float yMult;
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
	glm::mat4 ViewMatrix = glm::translate(glm::mat4(), glm::vec3(-3.0f, 0.0f ,0.0f));


	renderer->GetRendergraph()->AddPass(PassDesc()
		.SetName("GPU Drawing Triangle")
		.SetInitialisationFunc([&descriptorSetKey](Tether& tether)
		{
			tether.GetDescriptorCache()->WriteBuffer(descriptorSetKey, "info");
		})
		.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context)
		{
			{
				ImGui::SliderFloat("Time Multiplier", &timeMult, 1, 15);
				ImGui::SliderFloat("X Scale", &info.xMult, 1, 25);
				ImGui::SliderFloat("Y Scale", &info.yMult, 1, 25);
				info.yMult = info.xMult;
			}

			
			static auto startTime = std::chrono::high_resolution_clock::now();
			auto currentTime = std::chrono::high_resolution_clock::now();
			info.time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count() * timeMult;
			
			context.GetDescriptorSetCache()->SetResource(descriptorSetKey, "info", &info, sizeof(Information));

			context.GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetDefaultRenderpass(), context.GetSwapchainExtent(), VertexAttributes{}, DepthSettings::Disabled(), { BlendSettings::Mixed() }, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				descriptorSetKey.program);

			context.GetDescriptorSetCache()->BindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorSetKey);

			vkCmdDraw(buffer, 3, 1, 0, 0);
		}));


	gui.AddToGraph(renderer->GetRendergraph());

	renderer->GetRendergraph()->Initialise();

	while (renderer->Run()) { renderer->GetRendergraph()->Execute(); }

	vkDeviceWaitIdle(*renderer->GetDevice());

	delete program;
	delete gui.key.program;
}
