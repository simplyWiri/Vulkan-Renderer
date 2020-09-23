#define VMA_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS


#include "Renderer/Vulkan/Core.h"
#include "imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include "Utils/GUI.h"

using namespace Renderer;

std::vector<Vertex> vertices = {
	{ { -0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f } }, { { 0.5f, -0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } }, { { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } }, { { -0.5f, 0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f } },
	{ { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } }, { { 0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } }, { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f } }, { { -0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f } }
};

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0,
	// right
	1, 5, 6, 6, 2, 1,
	// back
	7, 6, 5, 5, 4, 7,
	// left
	4, 0, 3, 3, 7, 4,
	// bottom
	4, 5, 1, 1, 0, 4,
	// top
	3, 2, 6, 6, 7, 3
};

struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

int main()
{
	auto renderer = std::make_unique<Core>(1200, 800, "Window");
	GUI gui;

	renderer->Initialise();
	gui.initialise(renderer.get());

	//Memory::Buffer buff = renderer->GetRendergraph()->allocator->AllocBuffer((sizeof(Vertex) * vertices.size() + sizeof(uint16_t) * indices.size()) * 3, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	Buffer* tribuffer = new Buffer(renderer->GetAllocator(), (sizeof(Vertex) * vertices.size() + sizeof(uint16_t) * indices.size()) * 3, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	tribuffer->load((void*)vertices.data(), sizeof(Vertex) * vertices.size());
	tribuffer->load((void*)indices.data(), sizeof(uint16_t) * indices.size(), sizeof(Vertex) * vertices.size());

	auto program = renderer->GetShaderManager()->getProgram({ renderer->GetShaderManager()->defaultVertex(), renderer->GetShaderManager()->defaultFragment() });
	program->initialiseResources(renderer->GetDevice()->GetDevice());

	DescriptorSetKey descriptorSetKey = { program };
	const auto vert = Vertex::defaultVertex();

	renderer->GetRendergraph()->AddPass(PassDesc()
		.SetName("GPU Drawing Triangle")
		.SetInitialisationFunc([&descriptorSetKey](Tether& tether)
			{
				tether.GetDescriptorCache()->WriteBuffer(descriptorSetKey, "ubo");
			})
		.SetRecordFunc([&renderer, &tribuffer, &descriptorSetKey, &vert](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context)
			{
				{
					static auto startTime = std::chrono::high_resolution_clock::now();

					auto currentTime = std::chrono::high_resolution_clock::now();
					float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

					UniformBufferObject ubo = {};
					ubo.model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
					ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
					ubo.proj = glm::perspective(glm::radians(45.0f), context.GetExtent().width / static_cast<float>(context.GetExtent().height), 0.1f, 10.0f);
					ubo.proj[1][1] *= -1;

					renderer->GetDescriptorSetCache()->SetResource(descriptorSetKey, "ubo", &ubo, sizeof(UniformBufferObject));

					auto modifier = std::sin(frameInfo.frameIndex / 720.0f);
					auto modifiedVerts = std::vector<Vertex>(vertices.size());
					std::transform(vertices.begin(), vertices.end(), modifiedVerts.begin(), [&modifier](Vertex v)
						{
							v.pos *= modifier + 1.2;
							return v;
						});

					tribuffer->load(modifiedVerts.data(), sizeof(Vertex) * modifiedVerts.size());
				}

				renderer->GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.getDefaultRenderpass(), context.GetExtent(), vert, DepthSettings::DepthTest(), { BlendSettings::Opaque() },
					VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, descriptorSetKey.program);

				VkDeviceSize offsets[] = { 0 };

				vkCmdBindVertexBuffers(buffer, 0, 1, &tribuffer->getBuffer(), offsets);
				vkCmdBindIndexBuffer(buffer, tribuffer->getBuffer(), sizeof(Vertex) * vertices.size(), VK_INDEX_TYPE_UINT16);

				renderer->GetDescriptorSetCache()->BindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorSetKey);

				vkCmdDrawIndexed(buffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
			}));


	gui.AddToGraph(renderer->GetRendergraph());

	renderer->GetRendergraph()->Initialise();

	while (renderer->Run())
	{
		renderer->GetRendergraph()->Execute();
	}

	vkDeviceWaitIdle(*renderer->GetDevice());

	delete tribuffer;
}
