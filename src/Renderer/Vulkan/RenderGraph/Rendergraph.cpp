#include "Rendergraph.h"

#include "../Core.h"
#include "imgui.h"

namespace Renderer
{
	Rendergraph::Rendergraph(Core* core)
	{
		this->core = core;

		buffers.resize(core->GetSwapchain()->GetFramesInFlight());

		VkCommandPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolCreateInfo.queueFamilyIndex = core->GetDevice()->GetIndices()->graphicsFamily;
		poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		auto success = vkCreateCommandPool(*core->GetDevice(), &poolCreateInfo, nullptr, &pool);
		Assert(success == VK_SUCCESS, "Failed to create command pool");

		VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
		commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocInfo.commandPool = pool;
		commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(buffers.size());

		success = vkAllocateCommandBuffers(*core->GetDevice(), &commandBufferAllocInfo, buffers.data());
		Assert(success == VK_SUCCESS, "Failed to allocate command buffers");

		depthImage = new Image(core->GetDevice(), core->GetSwapchain()->GetExtent(), VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	}

	Rendergraph::~Rendergraph()
	{
		vkDestroyCommandPool(*core->GetDevice(), pool, nullptr);
		delete depthImage;
	}

	void Rendergraph::Initialise()
	{
		extractGraphInformation();

		/* Todo at a later date*/
		//buildTransients();
		//mergePasses();
		//buildBarriers();
	}

	void Rendergraph::Execute()
	{
		core->GetAllocator()->BeginFrame();

		FrameInfo frameInfo;
		VkCommandBuffer buffer = buffers[core->GetSwapchain()->GetIndex()];

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		core->BeginFrame(buffer, frameInfo);

		ImGui::NewFrame();

		ImGui::SetNextWindowPos({ 5, 5 });
		ImGui::Begin("Information");

		ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / frameInfo.fps), frameInfo.fps);

		if (ImGui::CollapsingHeader("RenderGraph"))
		{
			ShowDebugVisualisation();
		}

		if (ImGui::CollapsingHeader("Allocations"))
		{
			core->GetAllocator()->DebugView();
		}
		
		ImGui::End();

		vkBeginCommandBuffer(buffer, &beginInfo);

		auto renderpass = core->GetRenderpassCache()->Get(RenderpassKey({ { core->GetSwapchain()->GetFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR } }, { depthImage->GetFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR }));


		core->GetFramebufferCache()->BeginPass(buffer, frameInfo.offset, { frameInfo.imageView, depthImage->GetView() }, renderpass, core->GetSwapchain()->GetExtent());

		GraphContext context;
		context.extent = core->GetSwapchain()->GetExtent();
		context.renderpass = renderpass;
		context.graph = this;

		for (auto& pass : passes)
		{
			context.passId = pass.taskName;
			pass.execute(buffer, frameInfo, context);
		}

		core->GetFramebufferCache()->EndPass(buffer);

		vkEndCommandBuffer(buffer);

		core->EndFrame(frameInfo);
		core->GetAllocator()->EndFrame();
	}

	void Rendergraph::Rebuild()
	{
		buffers.clear();
		buffers.resize(core->GetSwapchain()->GetFramesInFlight());

		VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
		commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocInfo.commandPool = pool;
		commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(buffers.size());

		auto success = vkAllocateCommandBuffers(*core->GetDevice(), &commandBufferAllocInfo, buffers.data());
		Assert(success == VK_SUCCESS, "Failed to allocate command buffers");

		delete depthImage;
		depthImage = new Image(core->GetDevice(), core->GetSwapchain()->GetExtent(), VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	}

	void Rendergraph::AddPass(PassDesc passDesc)
	{
		if (passDesc.target.has_value() || passDesc.extent.has_value()) uniquePasses.emplace_back(passDesc);
		else passes.emplace_back(passDesc);
	}

	void Rendergraph::extractGraphInformation()
	{
		auto ProcessPass = [&](PassDesc& pass)
		{
			Tether passResources;
			passResources.descriptorCache = core->GetDescriptorSetCache();

			if (pass.initialisation != nullptr) pass.initialisation(passResources);
		};

		for (auto& pass : passes) { ProcessPass(pass); }
		for (auto& pass : uniquePasses) { ProcessPass(pass); }
	}

	void Rendergraph::validateGraph() { }

	void Rendergraph::mergePasses() { }

	void Rendergraph::buildTransients() { }

	void Rendergraph::buildBarriers() { }
	void Rendergraph::ShowDebugVisualisation()
	{
		int i = 0;

		ImGui::BeginChild("RenderPasses");
		
		ImGui::Columns(2, "Render Passes", true);
		ImGui::Text("Name");
		ImGui::NextColumn();
		ImGui::Text("Order");
		ImGui::NextColumn();
		ImGui::Separator();

		static int selected = -1;

		for (auto& pass : passes)
		{
			ImGui::BeginGroup();
			char label[32];
			sprintf_s(label, "%s", pass.taskName.c_str());
			if (ImGui::Selectable(label, selected == i, ImGuiSelectableFlags_SpanAllColumns))
				selected = selected == i ? -1 : i;

			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("View Statistics");

			ImGui::NextColumn();
			ImGui::Text("%d", i++);
			ImGui::NextColumn();
			ImGui::Separator();
			ImGui::EndGroup();
		}

		ImGui::EndChild();
	}
}
