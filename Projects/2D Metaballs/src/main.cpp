#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS


#include "Renderer/Core.h"
#include <glm/gtc/matrix_transform.hpp>
#include "GUI.h"
#include "math.h"

using namespace Renderer;

struct Circle
{
	glm::vec3 position;
	float radius;
};

float lerp(float a, float b, float f)
{
    return a + f * (b - a);
}

int main()
{
	Settings s = {};
	s.width = 1200;
	s.height = 840;
	s.vsync = true;

	auto renderer = std::make_unique<Core>(s);
	GUI gui;

	renderer->Initialise();
	gui.initialise(renderer.get());

	auto program = renderer->GetShaderManager()->getProgram({ renderer->GetShaderManager()->fullScreenTri(), renderer->GetShaderManager()->get(ShaderType::Fragment, "resources/metaballs.frag") });
	program->InitialiseResources(renderer->GetDevice()->GetDevice());

	DescriptorSetKey descriptorSetKey = { program };

	Circle circles[] = { { { .3, .3, 0}, .1 },  { { 0.5, 0.5, 0}, .1 }, { { .7, .7, 0}, .1 } };


	renderer->GetRenderGraph()->AddPass(RenderGraphBuilder()
		.SetName("GPU Drawing Triangle")
		.SetInitialisationFunc([&descriptorSetKey](Tether& tether)
		{
			tether.GetDescriptorCache()->WriteBuffer(descriptorSetKey, "circles");
		})
		.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context)
		{
			{
				static auto startTime = std::chrono::high_resolution_clock::now();
				auto currentTime = std::chrono::high_resolution_clock::now();
				float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
				float sin = std::sin(time);
				float cos = std::cos(time);
				
				for(int i = 0; i < 3; i++)
				{
					circles[i].position.x =	lerp(.4f, .6f, i * sin);
					circles[i].position.y = lerp(.4f, .6f, cos);
				}
			}
			
			context.GetDescriptorSetCache()->SetResource(descriptorSetKey, "circles", &circles[0], sizeof(Circle) * 3);
			
			context.GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetDefaultRenderpass(), context.GetSwapchainExtent(), VertexAttributes{}, DepthSettings::Disabled(), { BlendSettings::Mixed() }, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				descriptorSetKey.program);

			context.GetDescriptorSetCache()->BindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorSetKey);

			vkCmdDraw(buffer, 3, 1, 0, 0);
		}));


	gui.AddToGraph(renderer->GetRenderGraph());

	renderer->GetRenderGraph()->Initialise();

	while (renderer->Run()) { renderer->GetRenderGraph()->Execute(); }

	vkDeviceWaitIdle(*renderer->GetDevice());

	delete program;
	delete gui.key.program;
}
