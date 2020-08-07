#pragma once
#include "imgui.h"
#include "../Renderer/Vulkan/Core.h"

namespace Renderer

{
	class GUI
	{
		public:
			GUI() { ImGui::CreateContext(); }
			~GUI() { ImGui::DestroyContext(); }

			void initialise(Core* core)
			{
				this->core = core;

				auto program = core->GetShaderManager()->getProgram({ core->GetShaderManager()->get(ShaderType::Vertex, "resources/ImguiVertex.vert"), core->GetShaderManager()->get(ShaderType::Fragment, "resources/ImguiFragment.frag") });
				program.initialiseResources(core->GetDevice()->getDevice());

				imguiVerts = VertexAttributes({ { sizeof(ImDrawVert), 0 } }, {
					{ 0, 0, VertexAttributes::Type::vec2, offsetof(ImDrawVert, pos) }, { 0, 1, VertexAttributes::Type::vec2, offsetof(ImDrawVert, uv) }, { 0, 2, VertexAttributes::Type::colour32, offsetof(ImDrawVert, col) }
				});

				ImGuiIO& io = ImGui::GetIO();

				unsigned char* fontData;
				int texWidth, texHeight;
				io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
				VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

				auto stagingBuffer = new Buffer(core->GetAllocator(), uploadSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

				stagingBuffer->load(fontData, uploadSize);

				fontImage = new Image(core->GetDevice(), { static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight) }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
					VMA_MEMORY_USAGE_GPU_ONLY);

				auto cmd = core->getCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

				core->setImageLayout(cmd, fontImage->getImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, fontImage->getSubresourceRange(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = fontImage->getSubresourceRange().aspectMask;
				bufferCopyRegion.imageSubresource.layerCount = fontImage->getSubresourceRange().layerCount;
				bufferCopyRegion.imageExtent = fontImage->getExtent3D();

				vkCmdCopyBufferToImage(cmd, stagingBuffer->getBuffer(), fontImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

				core->setImageLayout(cmd, fontImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, fontImage->getSubresourceRange(), VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

				core->flushCommandBuffer(cmd);

				sampler = new Sampler(core->GetDevice()->getDevice(), VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR);
			}


			void AddToGraph(Rendergraph* graph)
			{
				//graph->AddPass(PassDesc().SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& info, GraphContext& context)
				//{
				//	DescriptorSetKey key = { program };
				//	ImGuiIO& io = ImGui::GetIO();

				//	core->GetGraphicsPipelineCache()->bindGraphicsPipeline(buffer, context.getDefaultRenderpass(), context.extent, imguiVerts, DepthSettings::Disabled(), { BlendSettings::AlphaBlend() }, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				//		program);
				//	core->GetDescriptorSetCache()->bindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, key, info.offset);

				//	VkViewport viewport = { ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 0.0f, 1.0f };
				//	vkCmdSetViewport(buffer, 0, 1, &viewport);

				//	// UI scale and translate via push constants
				//	pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
				//	pushConstBlock.translate = glm::vec2(-1.0f);
				//	vkCmdPushConstants(buffer, program.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

				//	// Render commands
				//	ImDrawData* imDrawData = ImGui::GetDrawData();
				//	int32_t vertexOffset = 0;
				//	int32_t indexOffset = 0;

				//	if (imDrawData->CmdListsCount > 0)
				//	{
				//		VkDeviceSize offsets[1] = { 0 };
				//		vkCmdBindVertexBuffers(buffer, 0, 1, &vertexBuffer.buffer, offsets);
				//		vkCmdBindIndexBuffer(buffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

				//		for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
				//		{
				//			const ImDrawList* cmd_list = imDrawData->CmdLists[i];
				//			for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
				//			{
				//				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
				//				VkRect2D scissorRect;
				//				scissorRect.offset.x = std::max(static_cast<int32_t>(pcmd->ClipRect.x), 0);
				//				scissorRect.offset.y = std::max(static_cast<int32_t>(pcmd->ClipRect.y), 0);
				//				scissorRect.extent.width = static_cast<uint32_t>(pcmd->ClipRect.z - pcmd->ClipRect.x);
				//				scissorRect.extent.height = static_cast<uint32_t>(pcmd->ClipRect.w - pcmd->ClipRect.y);
				//				vkCmdSetScissor(buffer, 0, 1, &scissorRect);
				//				vkCmdDrawIndexed(buffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
				//				indexOffset += pcmd->ElemCount;
				//			}
				//			vertexOffset += cmd_list->VtxBuffer.Size;
				//		}
				//	}
				//}));
			}

		private:
			struct PushConstBlock
			{
				glm::vec2 scale;
				glm::vec2 translate;
			} pushConstBlock;

			Sampler* sampler;
			Image* fontImage;
			ShaderProgram program;
			Core* core;

			VertexAttributes imguiVerts;
	};
}
