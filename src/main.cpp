#define VMA_IMPLEMENTATION

#include "Renderer/Vulkan/Core.h"
#include "imgui.h"
#include <glm/gtc/matrix_transform.hpp>

using namespace Renderer;

const std::vector<Vertex> vertices = {
{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
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

	Buffer* vertexBuffer;
	Buffer* indexBuffer;

	vertexBuffer = new Buffer(renderer->GetAllocator(), VkDeviceSize(64 * 64), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU);
	indexBuffer = new Buffer(renderer->GetAllocator(), VkDeviceSize(64 * 64), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU);

	vertexBuffer->load((void*)vertices.data(), sizeof(Vertex) * vertices.size());
	indexBuffer->load((void*)indices.data(), sizeof(uint16_t) * indices.size());

	auto& program = renderer->GetRendergraph()->getShaderManger().getProgram({renderer->GetRendergraph()->getShaderManger().defaultVertex(), renderer->GetRendergraph()->getShaderManger().defaultFragment()});
	program.initialiseResources(renderer->GetDevice()->getDevice());
	
	renderer->GetRendergraph()->AddPass(PassDesc()
		.SetName("Triangle")
		.SetInitialisationFunc([](Tether& tether) {})
		.SetRecordFunc([vertexBuffer, indexBuffer, program](VkCommandBuffer buffer, const FrameInfo& frameInfo, GraphContext& context) -> void
			{

				GraphicsPipelineKey key = {};
				key.renderpass = context.getDefaultRenderpass();
				key.extent = context.getExtent();
				key.depthSetting = DepthSettings::Disabled();
				key.blendSettings = { BlendSettings::Add() };
				key.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				key.program = program;

				DescriptorSetKey descriptorSetKey = {};
				descriptorSetKey.dLayout = key.program.getDescriptorLayout();
				descriptorSetKey.resources = context.graph->getShaderManger().defaultVertex()->getResources();

				{ // update our buffers
					static auto startTime = std::chrono::high_resolution_clock::now();

					auto currentTime = std::chrono::high_resolution_clock::now();
					float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

					UniformBufferObject ubo{};
					ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
					ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
					ubo.proj = glm::perspective(glm::radians(45.0f), context.getExtent().width / (float)context.getExtent().height, 0.1f, 10.0f);
					ubo.proj[1][1] *= -1;

					context.graph->getDescriptorSetCache().setResource(descriptorSetKey, "ubo", frameInfo.offset, &ubo, sizeof(UniformBufferObject));
				}

				context.graph->getGraphicsPipelineCache().bindGraphicsPipeline(buffer, key);

				VkBuffer vertexBuffers[] = { vertexBuffer->getBuffer() };
				VkDeviceSize offsets[] = { 0 };

				vkCmdBindVertexBuffers(buffer, 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(buffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT16);

				context.graph->getDescriptorSetCache().bindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, key.program.getPipelineLayout(), descriptorSetKey, frameInfo.offset);

				vkCmdDrawIndexed(buffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
			}));

	renderer->GetRendergraph()->Initialise();

	while (renderer->Run())
	{
		renderer->GetRendergraph()->Execute();
	}

	vkDeviceWaitIdle(*renderer->GetDevice());
}

//renderer->Rendergraph()->AddPass(PassDesc()
//	.SetName("ImGUI")
//	.SetInitialisationFunc([](Tether& tether)
//		{
//			// buffers
//			tether.RegisterVertexBuffer("VERTEX");
//			tether.RegisterIndexBuffer("INDEX");
//			
//			// pipeline
//			// this will create the pipeline, and the relevent descriptor sets // layouts
//			tether.RegisterPipeline(NULL, VkExtent2D(), DepthSettings::Disabled(), { BlendSettings::AlphaBlend() }, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, std::vector<std::shared_ptr<Shader>>());
//		}));
//.SetRecordFunc([](FrameInfo& info, GraphContext& context)
//	{
//		// info contains information about the frame
//		// context contains handles to resources, Ex. "IM_VERT_BUFF"
//
//		uint32_t listIndexOffset = 0;
//		uint32_t listVertexOffset = 0;
//
//		auto ImDrawData = ImGui::GetDrawData();
//
//		{ // update our vertex buffers with the new drawData from IMGUI
//			ImDrawVert* vertBufMemory = (ImDrawVert*)context.GetBuffer("VERTEX").Map();
//			ImDrawIdx* indexBufMemory = (ImDrawIdx*)context.GetBuffer("INDEX").Map();
//
//			for (int cmdListIndex = 0; cmdListIndex < ImDrawData->CmdListsCount; cmdListIndex++)
//			{
//				const ImDrawList* cmdList = ImDrawData->CmdLists[cmdListIndex];
//
//				memcpy(vertBufMemory, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
//				memcpy(indexBufMemory, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
//
//				vertBufMemory += cmdList->VtxBuffer.Size;
//				indexBufMemory += cmdList->IdxBuffer.Size;
//			}
//			context.GetBuffer("INDEX").UnMap();
//			context.GetBuffer("VERTEX").UnMap();
//		}
//
//		for (int cmdListIndex = 0; cmdListIndex < ImDrawData->CmdListsCount; cmdListIndex++)
//		{
//			const ImDrawList* cmdList = ImDrawData->CmdLists[cmdListIndex];
//			const ImDrawVert* vertexBufferData = cmdList->VtxBuffer.Data;
//			const ImDrawIdx* indexBufferData = cmdList->IdxBuffer.Data;
//
//			for (int cmdIndex = 0; cmdIndex < cmdList->CmdBuffer.Size; cmdIndex++)
//			{
//				const ImDrawCmd* drawCmd = &cmdList->CmdBuffer[cmdIndex];
//
//				auto texImageView = drawCmd->TextureId;
//				//auto texBinding = drawCallSetInfo->MakeImageSamplerBinding("tex", texImageView, imageSpaceSampler.get());
//
//				//auto drawCallSet = context.GetDescriptorSet(*drawCallSetInfo, {}, {}, { texBinding });
//				//auto cmdBuff = context.GetCommandBuffer();
//
//				//context.BindDescriptorSets();
//
//				//vkCmdBindVertexBuffers(cmdBuff, 0, 1, &context.VertexBuffer("VERT"), 0);
//				//vkCmdBindIndexBuffer(cmd, &context.IndexBuffer("INDEX"), 0, VK_INDEX_TYPE_UINT32);
//
//				//vkCmdDrawIndexed(cmdBuff, drawCmd->ElemCount, 1, listIndexOffset + drawCmd->IdxOffset, listVertexOffset + drawCmd->VtxOffset, 0);
//
//			}
//			listIndexOffset += cmdList->IdxBuffer.Size;
//			listVertexOffset += cmdList->VtxBuffer.Size;
//		}
//	}));