#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "Renderer/Core.h"
#include "Renderer/Memory/Allocator.h"
#include "Renderer/Memory/Buffer.h"

using namespace Renderer;

class DeferredCube
{
private:
	using Mesh = std::pair<std::vector<TexturedVertex>, std::vector<unsigned>>;

	Mesh Cube() {
		return Mesh(std::vector<TexturedVertex> {
				// back
				TexturedVertex{ {-1, -1, -1}, {0, 0, -1}, {-1, 0, 0}, {0, 1, 0}, {1, 1} }, TexturedVertex{ {1, 1, -1}, {0, 0, -1}, {-1, 0, 0}, {0, 1, 0}, {0, 0} },
				TexturedVertex{ {1, -1, -1}, {0, 0, -1}, {-1, 0, 0}, {0, 1, 0}, {0, 1} }, TexturedVertex{ {1, 1, -1}, {0, 0, -1}, {-1, 0, 0}, {0, 1, 0}, {0, 0} },
				TexturedVertex{ {-1, -1, -1}, {0, 0, -1}, {-1, 0, 0}, {0, 1, 0}, {1, 1} }, TexturedVertex{ {-1, 1, -1}, {0, 0, -1}, {-1, 0, 0}, {0, 1, 0}, {1, 0} },
				// front 
				TexturedVertex{ {-1, -1, 1}, {0, 0, 1}, {1, 0.0, 0}, {0, 1, 0}, {0, 1} }, TexturedVertex{ {1, -1, 1}, {0, 0, 1},{1, 0.0, 0}, {0, 1, 0}, {1, 1} },
				TexturedVertex{ {1, 1, 1}, {0, 0, 1}, {1, 0.0, 0}, {0, 1, 0}, {1, 0} }, TexturedVertex{ {1, 1, 1}, {0, 0, 1}, {1, 0.0, 0}, {0, 1, 0}, {1, 0} },
				TexturedVertex{ {-1, 1, 1}, {0, 0, 1}, {1, 0.0, 0}, {0, 1, 0}, {0, 0} }, TexturedVertex{ {-1, -1, 1}, {0, 0, 1}, {1, 0.0, 0}, {0, 1, 0}, {0, 1} },
				// left 
				TexturedVertex{ {-1, 1, -1}, {-1, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 0} }, TexturedVertex{ {-1, -1, -1}, {-1, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 1} },
				TexturedVertex{ {-1, 1, 1}, {-1, 0, 0}, {0, 0, 1}, {0, 1, 0}, {1, 0} }, TexturedVertex{ {-1, -1, -1}, {-1, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 1} },
				TexturedVertex{ {-1, -1, 1}, {-1, 0, 0}, {0, 0, 1}, {0, 1, 0}, {1, 1} }, TexturedVertex{ {-1, 1, 1}, {-1, 0, 0}, {0, 0, 1}, {0, 1, 0}, {1, 0} },
				// right 
				TexturedVertex{ {1, 1, 1}, {1, 0, 0}, {0, 0, -1}, {0, 1, 0}, {0, 0} }, TexturedVertex{ {1, -1, -1}, {1, 0, 0}, {0, 0, -1}, {0, 1, 0}, {1, 1} },
				TexturedVertex{ {1, 1, -1}, {1, 0, 0}, {0, 0, -1}, {0, 1, 0}, {1, 0} }, TexturedVertex{ {1, -1, -1}, {1, 0, 0}, {0, 0, -1}, {0, 1, 0}, {1, 1} },
				TexturedVertex{ {1, 1, 1}, {1, 0, 0}, {0, 0, -1}, {0, 1, 0}, {0, 0} }, TexturedVertex{ {1, -1, 1}, {1, 0, 0}, {0, 0, -1}, {0, 1, 0}, {0, 1} },
				// bottom 
				TexturedVertex{ {-1, -1, -1}, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}, {0, 1} }, TexturedVertex{ {1, -1, -1}, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}, {1, 1} },
				TexturedVertex{ {1, -1, 1}, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}, {1, 0} }, TexturedVertex{ {1, -1, 1}, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}, {1, 0} },
				TexturedVertex{ {-1, -1, 1}, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}, {0, 0} }, TexturedVertex{ {-1, -1, -1}, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}, {0, 1} },
				// top 
				TexturedVertex{ {-1, 1, -1}, {0, 1, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0} }, TexturedVertex{ {1, 1, 1}, {0, 1, 0}, {1, 0, 0}, {0, 0, -1}, {1, 1} },
				TexturedVertex{ {1, 1, -1}, {0, 1, 0}, {1, 0, 0}, {0, 0, -1}, {1, 0} }, TexturedVertex{ {1, 1, 1}, {0, 1, 0}, {1, 0, 0}, {0, 0, -1}, {1, 1} },
				TexturedVertex{ {-1, 1, -1}, {0, 1, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0} }, TexturedVertex{ {-1, 1, 1}, {0, 1, 0}, {1, 0, 0}, {0, 0, -1}, {0, 1} } },
			{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35 });
	}

	struct UniformBufferObject
	{
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

	struct PushConstData
	{
		uint32_t gPosition;
	    uint32_t gNormal;
	    uint32_t gAlbedo;
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

		auto prepare = shaderManager->MakeProgram({ shaderManager->Get(ShaderType::Vertex, "resources/deferred.vert"), shaderManager->Get(ShaderType::Fragment, "resources/deferred.frag") });
		auto resolve = shaderManager->MakeProgram({ shaderManager->Get(ShaderType::Vertex, "resources/fullscreen.vert"), shaderManager->Get(ShaderType::Fragment, "resources/resolve.frag") });
		prepare->InitialiseResources(renderer->GetDevice()->GetDevice());
		resolve->InitialiseResources(renderer->GetDevice()->GetDevice());

		const auto vert = VertexAttributes{
		{
					{ sizeof(TexturedVertex), 0 }
				}, 
	   {
					{ 0, 0, VertexAttributes::Type::vec3, offsetof(TexturedVertex, position) },
					{ 0, 1, VertexAttributes::Type::vec3, offsetof(TexturedVertex, normal) },
   					{ 0, 2, VertexAttributes::Type::vec2, offsetof(TexturedVertex, uv) }
				}
			};

		float fov = 45.0f;

		auto cube = Cube();
		auto* cubeBuffer = renderer->GetAllocator()->AllocateBuffer((sizeof(TexturedVertex) * cube.first.size() + sizeof(unsigned) * cube.second.size()), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		cubeBuffer->Load((void*)cube.first.data(), sizeof(TexturedVertex) * cube.first.size());
		cubeBuffer->Load((void*)cube.second.data(), sizeof(unsigned) * cube.second.size());


		renderer->GetRenderGraph()->AddPass("prepare", RenderGraph::QueueType::Graphics)
			.WriteImage("position", VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, RenderGraph::ImageInfo{ .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, .format = VK_FORMAT_R16G16B16A16_SFLOAT })
			.WriteImage("normal", VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, RenderGraph::ImageInfo{ .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, .format = VK_FORMAT_R16G16B16A16_SFLOAT })
			.WriteImage("color", VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, RenderGraph::ImageInfo{ .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, .format = VK_FORMAT_R8G8B8A8_UNORM })
			.WriteImage("depth",VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, RenderGraph::ImageInfo{ .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, .format = VK_FORMAT_D32_SFLOAT })
			.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, RenderGraph::GraphContext& context)
	          {
		          context.GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetRenderpass(), context.GetExtent(), vert, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, prepare);

		          VkDeviceSize offsets[] = { 0 };

		          vkCmdBindVertexBuffers(buffer, 0, 1, &cubeBuffer->GetResourceHandle(), offsets);
		          vkCmdBindIndexBuffer(buffer, cubeBuffer->GetResourceHandle(), sizeof(glm::vec3) * 3 * cube.first.size(), VK_INDEX_TYPE_UINT16);

		          vkCmdDrawIndexed(buffer, cube.second.size(), 1, 0, 0, 0);
	          });

		renderer->GetRenderGraph()->AddPass("resolve", RenderGraph::QueueType::Graphics)
			.ReadImage("position", VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT)
			.ReadImage("normal", VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT)
			.ReadImage("color", VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT)
			.WriteToBackbuffer(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT)
			.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, RenderGraph::GraphContext& context)
			{
				context.GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetRenderpass(), context.GetExtent(), vert, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, resolve);

				vkCmdPushConstants(buffer, resolve->getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t) * 3, )
				
				vkCmdDraw(buffer, 3, 1, 0, 0);
			});


		renderer->GetRenderGraph()->Build();

		while (renderer->Run())
		{
			renderer->GetRenderGraph()->Execute();
		}

		vkDeviceWaitIdle(*renderer->GetDevice());

		delete resolve;
		delete prepare;
	}
};
