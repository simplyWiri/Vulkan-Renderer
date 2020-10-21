#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS


#include "Renderer/Core.h"
#include "Utils/GUI.h"
#include "Renderer/RenderGraph/Tether.h"
#include "Renderer/RenderGraph/GraphContext.h"

using namespace Renderer;

unsigned char* data;
unsigned char* backbuffer;

inline unsigned char CountAliveCells(size_t leftX, size_t middleX, size_t rightX, size_t upperY, size_t middleY, size_t lowerY)
{
  return data[leftX + upperY] + data[middleX + upperY] + data[rightX + upperY]
	   + data[leftX + middleY] + data[rightX + middleY]
	   + data[leftX + lowerY] + data[middleX + lowerY] + data[rightX + lowerY];
}

int main()
{
	size_t width = 1200;
	size_t height = 840;

	data = new unsigned char[width * height];
	backbuffer = new unsigned char[width * height];

	Settings s = {};
	s.width = 1200;
	s.height = 840;


	auto renderer = std::make_unique<Core>(s);
	GUI gui;

	renderer->Initialise();
	gui.initialise(renderer.get());

	auto program = renderer->GetShaderManager()->getProgram({ renderer->GetShaderManager()->fullScreenTri(), renderer->GetShaderManager()->get(ShaderType::Fragment, "resources/GameOfLife.frag") });
	program->InitialiseResources(renderer->GetDevice()->GetDevice());

	DescriptorSetKey descriptorSetKey = { program };

	/*
	 * The idea here is relatively simple; Have a texture created on the GPU via compute, then sample from it using a fragment shader
	 */

	renderer->GetRendergraph()->AddPass("CPU-Game-Of-Life", RenderGraphQueue::AsyncCompute)
		.SetInitialisationFunc([&descriptorSetKey](Tether& tether)
		{
			auto info = ImageInfo{};
			info.format = VK_FORMAT_R16G16B16A16_SFLOAT;
			info.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			tether.AddWriteDependencyImage("compute-gol", VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, info);
		}).SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context)
		{
			for (size_t y = 0; y < height; ++y)
			{
				size_t upperY = ((y + height - 1) % height) * width;
				size_t middleY = y * width;
				size_t lowerY = ((y + 1) % height) * width;

				for (size_t x = 0; x < width; ++x)
				{
					size_t leftX = (x + width - 1) % width;
					size_t rightX = (x + 1) % width;

					unsigned char aliveCells = CountAliveCells(leftX, x, rightX, upperY, middleY, lowerY);
					backbuffer[middleY + x] = aliveCells == 3 || (aliveCells == 2 && data[x + middleY]) ? 1 : 0;
				}
			}

			std::swap(data, backbuffer);
		});

	renderer->GetRendergraph()->AddPass("GPU Game Of Life", RenderGraphQueue::Graphics)
		.SetInitialisationFunc([&descriptorSetKey](Tether& tether)
		{
			tether.AddReadDependencyImage("compute-gol", VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			tether.AddWriteDependencyImage(tether.graph->GetBackBuffer(), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT);
		}).SetRecordFunc(
		[&](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context)
		{
			// Draw a fullscreen triangle (runs the fragment shader on every pixel).
			
			context.GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetDefaultRenderpass(), context.GetSwapchainExtent(), VertexAttributes{}, DepthSettings::Disabled(), { BlendSettings::Mixed() },
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, descriptorSetKey.program);

			context.GetDescriptorSetCache()->BindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorSetKey);

			vkCmdDraw(buffer, 3, 1, 0, 0);
		});

	gui.AddToGraph(renderer->GetRendergraph());

	renderer->GetRendergraph()->Initialise();

	while (renderer->Run()) { renderer->GetRendergraph()->Execute(); }

	vkDeviceWaitIdle(*renderer->GetDevice());

	delete[] data;
	delete[] backbuffer;
	delete program;
}
