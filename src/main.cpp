#define VMA_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS


#include "Renderer/Vulkan/Core.h"
#include "imgui.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace Renderer;

const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f} },
	{{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f} },
	{{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f} },
	{{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f} },

	{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f} },
	{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f} },
	{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f} },
	{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f} }
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

int main()
{
	auto renderer = std::make_unique<Core>(640, 400, "Window");

	renderer->Initialise();

	Buffer* tribuffer = new Buffer(renderer->GetAllocator(), (sizeof(Vertex) * vertices.size() + sizeof(uint16_t) * indices.size()), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU);

	tribuffer->load((void*)vertices.data(), sizeof(Vertex) * vertices.size());
	tribuffer->load((void*)indices.data(), sizeof(uint16_t) * indices.size(), sizeof(Vertex) * vertices.size());

	auto program = renderer->GetShaderManager()->getProgram({ renderer->GetShaderManager()->defaultVertex(), renderer->GetShaderManager()->defaultFragment() });
	program.initialiseResources(renderer->GetDevice()->getDevice());

	auto guiprogram = renderer->GetShaderManager()->getProgram({ renderer->GetShaderManager()->get(ShaderType::Vertex, "resources/ImguiVertex.vert"), renderer->GetShaderManager()->get(ShaderType::Fragment, "resources/ImguiFragment.frag") });
	guiprogram.initialiseResources(renderer->GetDevice()->getDevice());

	renderer->GetRendergraph()->AddPass(PassDesc()
		.SetName("GPU Drawing Triangle")
		.SetInitialisationFunc([](Tether& tether)
			{
			})
		.SetRecordFunc([&renderer, tribuffer, program](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context) -> void
			{
				DescriptorSetKey descriptorSetKey = { program };

				{
					static auto startTime = std::chrono::high_resolution_clock::now();

					auto currentTime = std::chrono::high_resolution_clock::now();
					float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

					UniformBufferObject ubo = {};
					ubo.model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
					ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
					ubo.proj = glm::perspective(glm::radians(45.0f), context.getExtent().width / (float)context.getExtent().height, 0.1f, 10.0f);
					ubo.proj[1][1] *= -1;

					renderer->GetDescriptorSetCache()->setResource(descriptorSetKey, "ubo", frameInfo.offset, &ubo, sizeof(UniformBufferObject));
				}

				renderer->GetGraphicsPipelineCache()->bindGraphicsPipeline(buffer, context.getDefaultRenderpass(), context.getExtent(), Vertex::defaultVertex(), DepthSettings::DepthTest(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, program);

				VkDeviceSize offsets[] = { 0 };

				vkCmdBindVertexBuffers(buffer, 0, 1, &tribuffer->getBuffer(), offsets);
				vkCmdBindIndexBuffer(buffer, tribuffer->getBuffer(), offsets[0] + sizeof(Vertex) * vertices.size(), VK_INDEX_TYPE_UINT16);

				renderer->GetDescriptorSetCache()->bindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorSetKey, frameInfo.offset);

				vkCmdDrawIndexed(buffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
			}));

	//renderer->GetRendergraph()->AddPass(PassDesc()
	//	.SetName("ImGUI")
	//	.SetInitialisationFunc([](Tether& tether)
	//		{
	//		})
	//	.SetRecordFunc([](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context)
	//		{
	//			// info contains information about the frame
	//			// context contains handles to resources, Ex. "IM_VERT_BUFF"

	//			uint32_t listIndexOffset = 0;
	//			uint32_t listVertexOffset = 0;

	//			auto ImDrawData = ImGui::GetDrawData();

	//			{ // update our vertex buffers with the new drawData from IMGUI
	//				ImDrawVert* vertBufMemory = (ImDrawVert*)context.GetBuffer("VERTEX").Map();
	//				ImDrawIdx* indexBufMemory = (ImDrawIdx*)context.GetBuffer("INDEX").Map();

	//				for (int cmdListIndex = 0; cmdListIndex < ImDrawData->CmdListsCount; cmdListIndex++)
	//				{
	//					const ImDrawList* cmdList = ImDrawData->CmdLists[cmdListIndex];

	//					memcpy(vertBufMemory, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
	//					memcpy(indexBufMemory, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

	//					vertBufMemory += cmdList->VtxBuffer.Size;
	//					indexBufMemory += cmdList->IdxBuffer.Size;
	//				}
	//				context.GetBuffer("INDEX").UnMap();
	//				context.GetBuffer("VERTEX").UnMap();
	//			}

	//			for (int cmdListIndex = 0; cmdListIndex < ImDrawData->CmdListsCount; cmdListIndex++)
	//			{
	//				const ImDrawList* cmdList = ImDrawData->CmdLists[cmdListIndex];
	//				const ImDrawVert* vertexBufferData = cmdList->VtxBuffer.Data;
	//				const ImDrawIdx* indexBufferData = cmdList->IdxBuffer.Data;

	//				for (int cmdIndex = 0; cmdIndex < cmdList->CmdBuffer.Size; cmdIndex++)
	//				{
	//					const ImDrawCmd* drawCmd = &cmdList->CmdBuffer[cmdIndex];

	//					auto texImageView = drawCmd->TextureId;
	//					//auto texBinding = drawCallSetInfo->MakeImageSamplerBinding("tex", texImageView, imageSpaceSampler.get());

	//					//auto drawCallSet = context.GetDescriptorSet(*drawCallSetInfo, {}, {}, { texBinding });

	//					context.BindDescriptorSets();

	//					VkDeviceSize offsets[] = { 0 };

	//					vkCmdBindVertexBuffers(buffer, 0, 1, &context.VertexBuffer("VERT"), offsets);
	//					vkCmdBindIndexBuffer(buffer, &context.IndexBuffer("INDEX"), 0, VK_INDEX_TYPE_UINT16);

	//					vkCmdDrawIndexed(buffer, drawCmd->ElemCount, 1, listIndexOffset + drawCmd->IdxOffset, listVertexOffset + drawCmd->VtxOffset, 0);

	//				}
	//				listIndexOffset += cmdList->IdxBuffer.Size;
	//				listVertexOffset += cmdList->VtxBuffer.Size;
	//			}
	//		}));

	renderer->GetRendergraph()->Initialise();

	while (renderer->Run())
	{
		renderer->GetRendergraph()->Execute();
	}

	vkDeviceWaitIdle(*renderer->GetDevice());
	tribuffer->~Buffer();
}

