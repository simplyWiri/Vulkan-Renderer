#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS


#include <imgui.h>

#include "Renderer/Core.h"
#include "Renderer/RenderGraph/GraphContext.h"
#include "Renderer/RenderGraph/PassDesc.h"

using namespace Renderer;


void GameOfLife(RenderGraph* graph, bool bloom = true);

enum class ProgramState { GameOfLife, WireWorld, GameOfLife3D, Elementary, BriansBrain, LangstonsAnt };

int main()
{
	Settings s{};
	s.width = 1200;
	s.height = 800;
	s.vsync = true;

	auto core = std::make_unique<Core>(s);
	auto state = ProgramState::GameOfLife;

	core->Initialise();
	core->InitialiseGui();

	auto* graph = core->GetRenderGraph();

	GameOfLife(graph, false);
	core->AddGuiPass();

	graph->Build();
	
	while (core->Run())
	{
		//if (ImGui::Button("Game Of Life"))
		//{
		//	if (state != ProgramState::GameOfLife)
		//	{
		//		graph->Clear();

		//		core->AddGuiPass();
		//		GameOfLife(graph);
		//	}

		//	state = ProgramState::GameOfLife;
		//}

		core->GetRenderGraph()->Execute();
	}
	
}

void GameOfLife(RenderGraph* graph, bool bloom)
{
	//static Shader* fragment = Utility::Shader::Get(ShaderType::Fragment, "resources/GameOfLife.frag");
	//static Shader* compute = Utility::Shader::Get(ShaderType::Compute, "resources/GameOfLife.comp");
	//
	graph->AddPass("Compute GOL", QueueType::AsyncCompute)
			.AddFeedbackImage("compute-gol", VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT)
			.AddWrittenImage("compute-gol", VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, 
				ImageInfo{
					.sizeType = ImageSize::Swapchain,
					.usage = VK_IMAGE_USAGE_STORAGE_BIT,
					.format = VK_FORMAT_R16G16B16A16_SINT,
					.layout = VK_IMAGE_LAYOUT_GENERAL })
				.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context)
				{
					// Dispatch
				});

	graph->AddPass("Fragment GOL", QueueType::Graphics)
			.AddReadImage("compute-gol", VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			.AddWrittenImage(bloom ? "fragment-gol" : graph->GetBackBuffer(),VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_SHADER_WRITE_BIT, {})
			.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context)
			{
				// Full Screen tri, render from image received from compute
			});

	if(!bloom) return;
	
	// Bloom
	graph->AddPass("Bloom-Colour GOL", QueueType::Graphics)
			.AddReadImage("fragment-gol", VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			.AddWrittenImage("bloom-colour",VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_SHADER_WRITE_BIT, {})
			.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context)
			{
				// Colour
			});

	graph->AddPass("Bloom-Blur GOL", QueueType::Graphics)
			.AddReadImage("bloom-colour", VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			.AddWrittenImage("bloom-blur", VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_SHADER_WRITE_BIT, {})
			.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context)
			{
				// Blur
			});

	graph->AddPass("Bloom-Output GOL", QueueType::Graphics)
			.AddReadImage("fragment-gol", VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			.AddReadImage("bloom-blur", VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			.AddWrittenImage(graph->GetBackBuffer(),VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_SHADER_WRITE_BIT, {})
			.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context)
			{
				// Final output
			});
	
}
