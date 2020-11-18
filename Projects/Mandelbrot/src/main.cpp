#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS


#include "Renderer/Core.h"
#include "Utils/GUI.h"

const int WIDTH = 3200; // Size of rendered mandelbrot set.
const int HEIGHT = 2400; // Size of renderered mandelbrot set.
const int WORKGROUP_SIZE = 32; // Workgroup size in compute shader.

using namespace Renderer;

int main()
{
	Settings s = {};
	s.width = 1200;
	s.height = 840;

	auto renderer = std::make_unique<Core>(s);
	GUI gui;

	renderer->Initialise();
	gui.initialise(renderer.get());

	auto program = renderer->GetShaderManager()->getProgram({ renderer->GetShaderManager()->fullScreenTri(), renderer->GetShaderManager()->get(ShaderType::Fragment, "resources/mandelbrot.comp") });
	program->InitialiseResources(renderer->GetDevice()->GetDevice());

	DescriptorSetKey descriptorSetKey = { program };

	float timeMult = 1;
	auto rg = renderer->GetRenderGraph();
	
	rg->AddPass(RenderGraphBuilder("Mandelbrot-Compute", rg) // Write to an image in Compute
		.SetInitialisationFunc([&descriptorSetKey](Tether& tether)
		{
			tether.AddWriteDependencyImage("compute-image", VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL);
		})
		.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context)
		{

		}));

	rg->AddPass(RenderGraphBuilder("Mandelbrot-Fragment", rg) // Draw to backbuffer based on compute results
		.SetInitialisationFunc([&](Tether& tether)
		{
			tether.AddReadDependencyImage("compute-image", VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		})
		.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context)
		{
			
		}));

	gui.AddToGraph(renderer->GetRenderGraph());

	renderer->GetRenderGraph()->Initialise();

	while (renderer->Run()) { renderer->GetRenderGraph()->Execute(); }

	vkDeviceWaitIdle(*renderer->GetDevice());

	delete program;
}
