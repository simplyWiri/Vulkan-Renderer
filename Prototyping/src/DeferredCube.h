#define VK_USE_PLATFORM_WIN32_KHR
//#define VK_ENABLE_BETA_EXTENSIONS
#define VOLK_IMPLEMENTATION

#include "glm/glm/ext/matrix_clip_space.hpp"
#include "glm/glm/ext/matrix_transform.hpp"

#include "Renderer/Core.h"
#include "Renderer/Memory/Allocator.h"
#include "Renderer/Memory/Buffer.h"

#include "volk/volk.h"

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

		
		auto rgBuilder = RenderGraph::GraphBuilder{};

		auto prepareKey = DescriptorSetKey { prepare };
		UniformBufferObject ubo = {};

		
		rgBuilder.AddPass("prepare", RenderGraph::QueueType::Graphics)
			.WriteImage("position", VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, RenderGraph::ImageInfo{ .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, .format = VK_FORMAT_R16G16B16A16_SFLOAT })
			.WriteImage("normal", VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, RenderGraph::ImageInfo{ .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, .format = VK_FORMAT_R16G16B16A16_SFLOAT })
			.WriteImage("color", VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, RenderGraph::ImageInfo{ .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, .format = VK_FORMAT_R8G8B8A8_UNORM })
			.WriteImage("depth",VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, RenderGraph::ImageInfo{ .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, .format = VK_FORMAT_D32_SFLOAT })
			.SetInitialisationFunc([&](RenderGraph::RenderGraph* graph)
			{
				renderer->GetDescriptorSetCache()->WriteBuffer(prepareKey, "VP");
				renderer->GetDescriptorSetCache()->WriteBuffer(prepareKey, "Model");
			})
			.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, RenderGraph::GraphContext& context)
	        {
				{
					static auto startTime = std::chrono::high_resolution_clock::now();

					auto currentTime = std::chrono::high_resolution_clock::now();
					float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

					ubo.model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
					ubo.view = lookAt(glm::vec3(0.0f, 1.5f, 3.5f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					ubo.proj = glm::perspective(glm::radians(45.0f), context.GetExtent().width / (float)context.GetExtent().height, 0.1f, 10.0f);
					ubo.proj[1][1] *= -1;

					auto a = {ubo.view, ubo.proj};
					renderer->GetDescriptorSetCache()->SetResource(prepareKey, "VP", &a, sizeof(glm::mat4) * 2);
					renderer->GetDescriptorSetCache()->SetResource(prepareKey, "Model", &ubo.model, sizeof(glm::mat4));
				}
				
		          renderer->GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetRenderpass(), context.GetExtent(), vert, DepthSettings::Disabled(), { BlendSettings::Add(), BlendSettings::Add(), BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, prepare);
				  renderer->GetDescriptorSetCache()->BindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, prepareKey);
				
		          VkDeviceSize offsets[] = { 0 };

		          vkCmdBindVertexBuffers(buffer, 0, 1, &cubeBuffer->GetResourceHandle(), offsets);
		          vkCmdBindIndexBuffer(buffer, cubeBuffer->GetResourceHandle(), sizeof(glm::vec3) * 3 * cube.first.size(), VK_INDEX_TYPE_UINT16);

		          vkCmdDrawIndexed(buffer, cube.second.size(), 1, 0, 0, 0);
	        });

		auto resolveKey = DescriptorSetKey { resolve };

		rgBuilder.AddPass("resolve", RenderGraph::QueueType::Graphics)
			.ReadImage("position", VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT)
			.ReadImage("normal", VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT)
			.ReadImage("color", VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT)
			.WriteToBackbuffer(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT)
			.SetInitialisationFunc([&](RenderGraph::RenderGraph* graph)
			{
				renderer->GetDescriptorSetCache()->WriteSampler(resolveKey, "gPosition", graph->GetImage("position"), VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				renderer->GetDescriptorSetCache()->WriteSampler(resolveKey, "gNormal", graph->GetImage("normal"), VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				renderer->GetDescriptorSetCache()->WriteSampler(resolveKey, "gAlbedoSpec", graph->GetImage("color"), VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				renderer->GetDescriptorSetCache()->WriteBuffer(resolveKey, "V");

			})
			.SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& frameInfo, RenderGraph::GraphContext& context)
			{
				auto pos = glm::vec3(0.0f, 1.5f, 3.5f);
				renderer->GetDescriptorSetCache()->SetResource(resolveKey, "V", &pos, sizeof(glm::vec3));
				
				renderer->GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetRenderpass(), context.GetExtent(), {}, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, resolve);

				// Send the indexes of the bindless textures to the fragment shader.
				renderer->GetDescriptorSetCache()->BindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, resolveKey);

				vkCmdDraw(buffer, 3, 1, 0, 0);
			});

		auto* rg = renderer->CreateRenderGraph(rgBuilder);

		while (renderer->Run())
		{
			rg->Execute();
		}

		vkDeviceWaitIdle(*renderer->GetDevice());

		delete resolve;
		delete prepare;
	}
};
