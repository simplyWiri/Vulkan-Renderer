

#include "Renderer/Core.h"
#include "Renderer/RenderGraph/Creation/GraphBuilder.h"

using namespace Renderer;

class Triangle
{
public:
	
	void Start()
	{
		Renderer::Settings s = {};
		s.width = 1280;
		s.height = 720;
		s.vsync = true;

		auto renderer = std::make_unique<Renderer::Core>(s);
		renderer->Initialise();

		auto* shaderManager = renderer->GetShaderManager();

		auto program = shaderManager->MakeProgram({ shaderManager->Get(ShaderType::Vertex, "resources/vertex.vert"), shaderManager->Get(ShaderType::Fragment, "resources/fragment.frag") });
		program->InitialiseResources();

		DescriptorSetKey descriptorSetKey = { program };
		const auto vert = VertexAttributes{};

		float fov = 45.0f;

		
		auto rgBuilder = RenderGraph::GraphBuilder{};

		rgBuilder.AddPass("Triangle", RenderGraph::QueueType::Graphics)
			.WriteToBackbuffer(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT)
			.SetInitialisationFunc([&](RenderGraph::RenderGraph* graph)
			{
				
			})
			.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, RenderGraph::GraphContext& context)
			{
				renderer->GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetRenderpass(), context.GetExtent(), vert, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, descriptorSetKey.program);
				vkCmdDraw(buffer, 3, 1, 0, 0);
			});


		auto rg = renderer->CreateRenderGraph(rgBuilder);

		while (renderer->Run())
		{
			rg->Execute();
		}

		vkDeviceWaitIdle(*renderer->GetDevice());

		delete program;
	}

};