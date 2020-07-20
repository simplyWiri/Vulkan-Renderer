#define VMA_IMPLEMENTATION

#include "Renderer/Vulkan/Core.h"
#include "imgui.h"

using namespace Renderer;

int main()
{
	auto renderer = std::make_unique<Core>(640, 400, "Window");

	renderer->Initialise();

	while (renderer->Run()) { }
	vkDeviceWaitIdle(renderer->device);
}

//renderer->Rendergraph()->AddPass(PassDesc()
//	.SetName("ImGUI")
//	.SetInitialisationFunc([](Tether& tether)
//		{
//			// buffers
//			tether.RegisterVertexBuffer("VERTEX");
//			tether.RegisterIndexBuffer("INDEX");

//			tether.RegisterShader(ShaderType::Vertex, "resources/VertexShader.vert");
//			tether.RegisterShader(ShaderType::Fragment, "resources/FragmentShader.frag");

//			// pipeline
//			// this will create the pipeline, and the relevent descriptor sets // layouts
//			tether.RegisterPipeline(NULL, VkExtent2D(), DepthSettings::Disabled(), { BlendSettings::AlphaBlend() }, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, std::vector<std::shared_ptr<Shader>>());

//			//tether.External(Type, "name")
//		}));
//.SetRecordFunc([](FrameInfo& info, GraphContext& context)
//	{
//		// info contains information about the frame
//		// context contains handles to resources, Ex. "IM_VERT_BUFF"

//		uint32_t listIndexOffset = 0;
//		uint32_t listVertexOffset = 0;

//		auto ImDrawData = ImGui::GetDrawData();

//		{ // update our vertex buffers with the new drawData from IMGUI
//			ImDrawVert* vertBufMemory = (ImDrawVert*)context.VertexBuffer("VERTEX").Map();
//			ImDrawIdx* indexBufMemory = (ImDrawIdx*)context.IndexBuffer("INDEX").Map();

//			for (int cmdListIndex = 0; cmdListIndex < ImDrawData->CmdListsCount; cmdListIndex++)
//			{
//				const ImDrawList* cmdList = ImDrawData->CmdLists[cmdListIndex];

//				memcpy(vertBufMemory, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
//				memcpy(indexBufMemory, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

//				vertBufMemory += cmdList->VtxBuffer.Size;
//				indexBufMemory += cmdList->IdxBuffer.Size;
//			}
//			context.IndexBuffer("INDEX").UnMap();
//			context.VertexBuffer("VERTEX").UnMap();
//		}

//		for (int cmdListIndex = 0; cmdListIndex < ImDrawData->CmdListsCount; cmdListIndex++)
//		{
//			const ImDrawList* cmdList = ImDrawData->CmdLists[cmdListIndex];
//			const ImDrawVert* vertexBufferData = cmdList->VtxBuffer.Data;
//			const ImDrawIdx* indexBufferData = cmdList->IdxBuffer.Data;

//			for (int cmdIndex = 0; cmdIndex < cmdList->CmdBuffer.Size; cmdIndex++)
//			{
//				const ImDrawCmd* drawCmd = &cmdList->CmdBuffer[cmdIndex];

//				auto texImageView = drawCmd->TextureId;
//				//auto texBinding = drawCallSetInfo->MakeImageSamplerBinding("tex", texImageView, imageSpaceSampler.get());

//				//auto drawCallSet = context.GetDescriptorSet(*drawCallSetInfo, {}, {}, { texBinding });
//				//auto cmdBuff = context.GetCommandBuffer();

//				//context.BindDescriptorSets();

//				//vkCmdBindVertexBuffers(cmdBuff, 0, 1, &context.VertexBuffer("VERT"), 0);
//				//vkCmdBindIndexBuffer(cmd, &context.IndexBuffer("INDEX"), 0, VK_INDEX_TYPE_UINT32);

//				//vkCmdDrawIndexed(cmdBuff, drawCmd->ElemCount, 1, listIndexOffset + drawCmd->IdxOffset, listVertexOffset + drawCmd->VtxOffset, 0);

//			}
//			listIndexOffset += cmdList->IdxBuffer.Size;
//			listVertexOffset += cmdList->VtxBuffer.Size;
//		}
//	}));