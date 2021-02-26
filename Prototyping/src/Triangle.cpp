#include "Renderer/Core.h"

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
		program->InitialiseResources(renderer->GetDevice()->GetDevice());

		DescriptorSetKey descriptorSetKey = { program };
		const auto vert = VertexAttributes{};

		float fov = 45.0f;
		
		
		renderer->GetRenderGraph()->AddPass("Triangle", RenderGraph::QueueType::Graphics)
			.WriteToBackbuffer(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT)
			.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, RenderGraph::GraphContext& context)
			{
				context.GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetRenderpass(), context.GetExtent(), vert, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, descriptorSetKey.program);
				vkCmdDraw(buffer, 3, 1, 0, 0);
			});


		renderer->GetRenderGraph()->Build();

		while (renderer->Run())
		{
			renderer->GetRenderGraph()->Execute();
		}

		vkDeviceWaitIdle(*renderer->GetDevice());

		delete program;
	}

};