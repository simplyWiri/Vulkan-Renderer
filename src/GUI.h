#pragma once
#include <implot.h>

#include "glfw3.h"
#include "imgui.h"
#include "Renderer/Core.h"
#include "Renderer/Memory/Allocator.h"
#include "Renderer/Resources/Sampler.h"
#include "Renderer/Memory/Buffer.h"
#include "Renderer/Memory/Image.h"

namespace Renderer

{
	class GUI
	{
	public:
		struct PushConstBlock
		{
			glm::vec2 scale;
			glm::vec2 translate;
		} pushConstBlock;

		Sampler* sampler;
		DescriptorSetKey key;
		Core* core;

		Memory::Buffer* buffers;
		Memory::Image* fontImage;

		const VertexAttributes imguiVerts = VertexAttributes({ { sizeof(ImDrawVert), 0 } }, {
			{ 0, 0, VertexAttributes::Type::vec2, offsetof(ImDrawVert, pos) }, { 0, 1, VertexAttributes::Type::vec2, offsetof(ImDrawVert, uv) }, { 0, 2, VertexAttributes::Type::colour32, offsetof(ImDrawVert, col) }
		});


	public:


		GUI()
		{
			ImGui::CreateContext();
			ImPlot::CreateContext();
		}

		~GUI()
		{
			ImPlot::DestroyContext();
			ImGui::DestroyContext();
			delete buffers;
			delete fontImage;
		}

		void initialise(Core* core)
		{
			this->core = core;

			ImGuiIO& io = ImGui::GetIO();
			io.DisplaySize = ImVec2(static_cast<float>(core->GetSwapchain()->GetExtent().width), static_cast<float>(core->GetSwapchain()->GetExtent().height));

			initKeymap();
			initCallbacks(core);
			setupImGuiColour();

			auto program = core->GetShaderManager()->getProgram({ core->GetShaderManager()->get(ShaderType::Vertex, "../../resources/ImguiVertex.vert"), core->GetShaderManager()->get(ShaderType::Fragment, "../../resources/ImguiFragment.frag") });
			program->InitialiseResources(core->GetDevice()->GetDevice());

			key = { program };

			unsigned char* fontData;
			int texWidth, texHeight;
			io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);

			auto stagingBuffer = core->GetAllocator()->AllocateBuffer(texWidth * texHeight * 4 * sizeof(char), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			stagingBuffer->Load(fontData, texWidth * texHeight * 4 * sizeof(char));

			fontImage = core->GetAllocator()->AllocateImage(VkExtent2D{ static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight) }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			{
				// transfer data from staging buffer -> image

				auto cmd = core->GetCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

				core->SetImageLayout(cmd, fontImage->GetResourceHandle(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, fontImage->GetSubresourceRange(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = fontImage->GetSubresourceRange().aspectMask;
				bufferCopyRegion.imageSubresource.layerCount = fontImage->GetSubresourceRange().layerCount;
				bufferCopyRegion.imageExtent = fontImage->GetExtent3D();

				vkCmdCopyBufferToImage(cmd, stagingBuffer->GetResourceHandle(), fontImage->GetResourceHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

				core->SetImageLayout(cmd, fontImage->GetResourceHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, fontImage->GetSubresourceRange(), VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

				core->FlushCommandBuffer(cmd);
				delete stagingBuffer;
			}

			sampler = new Sampler(core->GetDevice()->GetDevice(), VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR);

			buffers = core->GetAllocator()->AllocateBuffer(1024 * sizeof(float), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		}

		void updateBuffers(ImDrawData* imDrawData)
		{
			if (imDrawData->CmdListsCount == 0) return;

			if (buffers->GetSize() == 0 || buffers->GetSize() < imDrawData->TotalVtxCount * sizeof(ImDrawVert) + imDrawData->TotalIdxCount * sizeof(ImDrawIdx))
			{
				vkQueueWaitIdle(core->GetDevice()->queues.graphics);
				auto usage = buffers->GetUsageFlags();
				auto flags = buffers->GetMemoryFlags();
				delete buffers;
				buffers = core->GetAllocator()->AllocateBuffer(imDrawData->TotalVtxCount * sizeof(ImDrawVert) + imDrawData->TotalIdxCount * sizeof(ImDrawIdx), usage, flags);
			}

			auto vtxDst = buffers->Map();
			auto idxDst = vtxDst + imDrawData->TotalVtxCount * sizeof(ImDrawVert);

			for (int n = 0; n < imDrawData->CmdListsCount; n++)
			{
				const ImDrawList* cmd_list = imDrawData->CmdLists[n];
				memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
				memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
				vtxDst += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
				idxDst += cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
			}

			buffers->Unmap();
		}

		void AddToGraph(Rendergraph* graph)
		{
			graph->AddPass(PassDesc().SetName("ImGui-CPU").SetRecordFunc([&](VkCommandBuffer buffer, const FrameInfo& info, GraphContext& context)
			{
				ImGuiIO& io = ImGui::GetIO();
				io.DisplaySize = ImVec2(static_cast<float>(context.GetSwapchainExtent().width), static_cast<float>(context.GetSwapchainExtent().height));

				auto window = core->GetSwapchain()->GetWindow();

				double mouseX, mouseY;
				glfwGetCursorPos(window, &mouseX, &mouseY);

				io.MousePos = ImVec2(static_cast<float>(mouseX), static_cast<float>(mouseY));
				//io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
				//io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
				//io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
				//io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];

				glfwSetInputMode(window, GLFW_CURSOR, io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

				ImGui::Render();

				auto* drawData = ImGui::GetDrawData();
				updateBuffers(drawData);

				pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
				pushConstBlock.translate = glm::vec2(-1.0f);
			}));

			graph->AddPass(
				PassDesc().SetName("ImGui-Render").SetInitialisationFunc([&](Tether& tether) { tether.GetDescriptorCache()->WriteSampler(key, "sTexture", fontImage, sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL); }).SetRecordFunc(
					[&](VkCommandBuffer buffer, const FrameInfo& info, GraphContext& context)
					{
						ImDrawData* imDrawData = ImGui::GetDrawData();
						if (imDrawData->CmdListsCount == 0) return;

						core->GetDescriptorSetCache()->BindDescriptorSet(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, key);

						core->GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetDefaultRenderpass(), context.GetSwapchainExtent(), imguiVerts, DepthSettings::Disabled(), { BlendSettings::AlphaBlend() },
							VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, key.program);

						VkViewport viewport = { 0, 0, static_cast<float>(context.GetSwapchainExtent().width), static_cast<float>(context.GetSwapchainExtent().height), 0.0f, 1.0f };
						vkCmdSetViewport(buffer, 0, 1, &viewport);

						vkCmdPushConstants(buffer, key.program->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

						VkDeviceSize offsets[1] = { 0 };
						vkCmdBindVertexBuffers(buffer, 0, 1, &buffers->GetResourceHandle(), offsets);
						vkCmdBindIndexBuffer(buffer, buffers->GetResourceHandle(), imDrawData->TotalVtxCount * sizeof(ImDrawVert), VK_INDEX_TYPE_UINT16);

						int32_t vertexOffset = 0;
						int32_t indexOffset = 0;

						for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
						{
							const ImDrawList* cmd_list = imDrawData->CmdLists[i];
							for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
							{
								const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
								VkRect2D scissorRect = {};
								scissorRect.offset.x = std::max(static_cast<int32_t>(pcmd->ClipRect.x), 0);
								scissorRect.offset.y = std::max(static_cast<int32_t>(pcmd->ClipRect.y), 0);
								scissorRect.extent.width = static_cast<uint32_t>(pcmd->ClipRect.z - pcmd->ClipRect.x);
								scissorRect.extent.height = static_cast<uint32_t>(pcmd->ClipRect.w - pcmd->ClipRect.y);
								vkCmdSetScissor(buffer, 0, 1, &scissorRect);
								vkCmdDrawIndexed(buffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);

								indexOffset += pcmd->ElemCount;
							}
							vertexOffset += cmd_list->VtxBuffer.Size;
						}
					}));
		}


	private:
		void initKeymap()
		{
			ImGuiIO& imguiIO = ImGui::GetIO();
			imguiIO.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
			imguiIO.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
			imguiIO.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
			imguiIO.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
			imguiIO.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
			imguiIO.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
			imguiIO.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
			imguiIO.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
			imguiIO.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
			imguiIO.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
			imguiIO.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
			imguiIO.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
			imguiIO.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
			imguiIO.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
			imguiIO.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
			imguiIO.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
			imguiIO.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
			imguiIO.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
			imguiIO.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
			imguiIO.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;
		}

		void initCallbacks(Core* core)
		{
			core->GetInputHandler()->RegisterKeyCallBack(keyCallback, InputPriority::GUI);
			core->GetInputHandler()->RegisterMouseClickCallBack(mouseButtonCallback, InputPriority::GUI);
			core->GetInputHandler()->RegisterMouseScrollCallBack(scrollCallback, InputPriority::GUI);
			core->GetInputHandler()->RegisterCharCallBack(charCallback, InputPriority::GUI);
		}

		static bool keyCallback(int key, int action)
		{
			ImGuiIO& imguiIO = ImGui::GetIO();
			imguiIO.KeysDown[key] = (action != GLFW_RELEASE);

			return false;
		}

		static bool charCallback(unsigned int c)
		{
			ImGuiIO& imguiIO = ImGui::GetIO();
			if (c > 0 && c < 0x10000) imguiIO.AddInputCharacter(static_cast<unsigned short>(c));

			return false;
		}

		static bool mouseButtonCallback(int button, int action)
		{
			ImGuiIO& imguiIO = ImGui::GetIO();
			if (button < 512) { imguiIO.MouseDown[button] = (action != GLFW_RELEASE); }

			return false;
		}

		static bool scrollCallback(float yOffset)
		{
			ImGuiIO& imguiIO = ImGui::GetIO();
			imguiIO.MouseWheel += yOffset;

			return false;
		}

		void setupImGuiColour()
		{
			ImVec4* colors = ImGui::GetStyle().Colors;
			colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
			colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
			colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
			colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
			colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
			colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
			colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.21f, 0.22f, 0.54f);
			colors[ImGuiCol_FrameBgHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.40f);
			colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 0.67f);
			colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
			colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
			colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
			colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
			colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
			colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
			colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
			colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
			colors[ImGuiCol_CheckMark] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
			colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
			colors[ImGuiCol_SliderGrabActive] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
			colors[ImGuiCol_Button] = ImVec4(0.44f, 0.44f, 0.44f, 0.40f);
			colors[ImGuiCol_ButtonHovered] = ImVec4(0.46f, 0.47f, 0.48f, 1.00f);
			colors[ImGuiCol_ButtonActive] = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
			colors[ImGuiCol_Header] = ImVec4(0.70f, 0.70f, 0.70f, 0.31f);
			colors[ImGuiCol_HeaderHovered] = ImVec4(0.70f, 0.70f, 0.70f, 0.80f);
			colors[ImGuiCol_HeaderActive] = ImVec4(0.48f, 0.50f, 0.52f, 1.00f);
			colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
			colors[ImGuiCol_SeparatorHovered] = ImVec4(0.72f, 0.72f, 0.72f, 0.78f);
			colors[ImGuiCol_SeparatorActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
			colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
			colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
			colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
			colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
			colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
			colors[ImGuiCol_PlotHistogram] = ImVec4(0.73f, 0.60f, 0.15f, 1.00f);
			colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
			colors[ImGuiCol_TextSelectedBg] = ImVec4(0.87f, 0.87f, 0.87f, 0.35f);
			colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
			colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
			colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
			colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);

			//ImGuiStyle* style = &ImGui::GetStyle();
			//ImVec4* colors = style->Colors;

			//style->WindowRounding = 2.0f; // Radius of window corners rounding. Set to 0.0f to have rectangular windows
			//style->ScrollbarRounding = 3.0f; // Radius of grab corners rounding for scrollbar
			//style->GrabRounding = 2.0f; // Radius of grabs corners rounding. Set to 0.0f to have rectangular slider grabs.
			//style->AntiAliasedLines = true;
			//style->AntiAliasedFill = true;
			//style->WindowRounding = 2;
			//style->ChildRounding = 2;
			//style->ScrollbarSize = 16;
			//style->ScrollbarRounding = 3;
			//style->GrabRounding = 2;
			//style->ItemSpacing.x = 10;
			//style->ItemSpacing.y = 4;
			//style->IndentSpacing = 22;
			//style->FramePadding.x = 6;
			//style->FramePadding.y = 4;
			//style->Alpha = 1.0f;
			//style->FrameRounding = 3.0f;

			//colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
			//colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
			//colors[ImGuiCol_WindowBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
			//colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			//colors[ImGuiCol_PopupBg] = ImVec4(0.93f, 0.93f, 0.93f, 0.98f);
			//colors[ImGuiCol_Border] = ImVec4(0.71f, 0.71f, 0.71f, 0.08f);
			//colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.04f);
			//colors[ImGuiCol_FrameBg] = ImVec4(0.71f, 0.71f, 0.71f, 0.55f);
			//colors[ImGuiCol_FrameBgHovered] = ImVec4(0.94f, 0.94f, 0.94f, 0.55f);
			//colors[ImGuiCol_FrameBgActive] = ImVec4(0.71f, 0.78f, 0.69f, 0.98f);
			//colors[ImGuiCol_TitleBg] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
			//colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.82f, 0.78f, 0.78f, 0.51f);
			//colors[ImGuiCol_TitleBgActive] = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
			//colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
			//colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.25f, 0.30f, 0.61f);
			//colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.90f, 0.90f, 0.90f, 0.30f);
			//colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.92f, 0.92f, 0.92f, 0.78f);
			//colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
			//colors[ImGuiCol_CheckMark] = ImVec4(0.184f, 0.407f, 0.193f, 1.00f);
			//colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
			//colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
			//colors[ImGuiCol_Button] = ImVec4(0.71f, 0.78f, 0.69f, 0.40f);
			//colors[ImGuiCol_ButtonHovered] = ImVec4(0.725f, 0.805f, 0.702f, 1.00f);
			//colors[ImGuiCol_ButtonActive] = ImVec4(0.793f, 0.900f, 0.836f, 1.00f);
			//colors[ImGuiCol_Header] = ImVec4(0.71f, 0.78f, 0.69f, 0.31f);
			//colors[ImGuiCol_HeaderHovered] = ImVec4(0.71f, 0.78f, 0.69f, 0.80f);
			//colors[ImGuiCol_HeaderActive] = ImVec4(0.71f, 0.78f, 0.69f, 1.00f);
			//colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
			//colors[ImGuiCol_SeparatorHovered] = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
			//colors[ImGuiCol_SeparatorActive] = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
			//colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
			//colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.45f);
			//colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
			//colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
			//colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
			//colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
			//colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
			//colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
			//colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
			//colors[ImGuiCol_DragDropTarget] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
			//colors[ImGuiCol_NavHighlight] = colors[ImGuiCol_HeaderHovered];
			//colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
		}
	};
}
