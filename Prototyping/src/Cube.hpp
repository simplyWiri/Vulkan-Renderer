#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "Renderer/Core.h"
#include "Renderer/Memory/Allocator.h"
#include "Renderer/Memory/Buffer.h"

using namespace Renderer;

class Cube
{
private:
	const std::vector<glm::vec3> vertices = {
		{-0.5f, -0.5f, 0.5f },
		{0.5f, -0.5f, 0.5f},
		{0.5f, 0.5f, 0.5f},
		{-0.5f, 0.5f, 0.5f },
		{-0.5f, -0.5f, -0.5f},
		{0.5f, -0.5f, -0.5f},
		{0.5f, 0.5f, -0.5f},
		{-0.5f, 0.5f, -0.5f} 
	};

	const std::vector<uint16_t> indices = {
			0, 1, 2,
			2, 3, 0,
			// right
			1, 5, 6,
			6, 2, 1,
			// back
			7, 6, 5,
			5, 4, 7,
			// left
			4, 0, 3,
			3, 7, 4,
			// bottom
			4, 5, 1,
			1, 0, 4,
			// top
			3, 2, 6,
			6, 7, 3
	};

	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

	
public:
	
	void Start()
	{
		Settings s = {};
		s.width = 1280;
		s.height = 720;
		s.vsync = true;

		auto renderer = std::make_unique<Renderer::Core>(s);
		renderer->Initialise();

		auto* shaderManager = renderer->GetShaderManager();

		auto program = shaderManager->MakeProgram({ shaderManager->Get(ShaderType::Vertex, "resources/cubeVertex.vert"), shaderManager->Get(ShaderType::Fragment, "resources/cubeFrag.frag") });
		program->InitialiseResources(renderer->GetDevice()->GetDevice());

		DescriptorSetKey descriptorSetKey = { program };
		const auto vert = VertexAttributes{
			{ { sizeof(glm::vec3), 0 } },
			{ { 0, 0, VertexAttributes::Type::vec3, 0}}
		};

		float fov = 45.0f;

		auto* cubeBuffer = renderer->GetAllocator()->AllocateBuffer((sizeof(glm::vec3) * vertices.size() + sizeof(uint16_t) * indices.size()), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		cubeBuffer->Load((void*)vertices.data(), sizeof(glm::vec3) * vertices.size());
		cubeBuffer->Load((void*)indices.data(), sizeof(uint16_t) * indices.size(), sizeof(glm::vec3) * vertices.size());

		renderer->GetDescriptorSetCache()->WriteBuffer(descriptorSetKey, "ubo");
		
		renderer->GetRenderGraph()->AddPass("Triangle", RenderGraph::QueueType::Graphics)
			.WriteToBackbuffer(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT)
			.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, RenderGraph::GraphContext& context)
			{
				{
					static auto startTime = std::chrono::high_resolution_clock::now();

					auto currentTime = std::chrono::high_resolution_clock::now();
					float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

					UniformBufferObject ubo = {};
					ubo.model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
					ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
					ubo.proj = glm::perspective(glm::radians(45.0f), context.GetExtent().width / (float)context.GetExtent().height, 0.1f, 10.0f);
					ubo.proj[1][1] *= -1;

					context.GetDescriptorSetCache()->SetResource(descriptorSetKey, "ubo", &ubo, sizeof(UniformBufferObject));
				}
				
				context.GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetRenderpass(), context.GetExtent(), vert, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, descriptorSetKey.program);

				VkDeviceSize offsets[] = { 0 };

				vkCmdBindVertexBuffers(buffer, 0, 1, &cubeBuffer->GetResourceHandle(), offsets);
				vkCmdBindIndexBuffer(buffer, cubeBuffer->GetResourceHandle(), sizeof(glm::vec3) * vertices.size(), VK_INDEX_TYPE_UINT16);

				context.GetDescriptorSetCache()->BindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorSetKey);
				
				vkCmdDrawIndexed(buffer, indices.size(), 1, 0, 0, 0);
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