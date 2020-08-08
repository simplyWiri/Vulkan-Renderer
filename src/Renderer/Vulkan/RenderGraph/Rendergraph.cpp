#include "Rendergraph.h"

#include "../Core.h"

namespace Renderer
{
	Rendergraph::Rendergraph(Core* core)
	{
		this->core = core;

		buffers.resize(core->GetSwapchain()->getFramesInFlight());

		VkCommandPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolCreateInfo.queueFamilyIndex = core->GetDevice()->getIndices()->graphicsFamily;
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

		depthImage = new Image(core->GetDevice(), core->GetSwapchain()->getExtent(), VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
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
		FrameInfo frameInfo;
		VkCommandBuffer buffer = buffers[core->GetSwapchain()->getIndex()];

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		core->BeginFrame(buffer, frameInfo);
		vkBeginCommandBuffer(buffer, &beginInfo);

		auto renderpass = core->GetRenderpassCache()->get(RenderpassKey({ { core->GetSwapchain()->getFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR } }, { depthImage->getFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR }));


		core->GetFramebufferCache()->BeginPass(buffer, frameInfo.offset, { frameInfo.imageView, depthImage->getView() }, renderpass, core->GetSwapchain()->getExtent());

		GraphContext context;
		context.extent = core->GetSwapchain()->getExtent();
		context.renderpass = renderpass;
		context.graph = this;

		for (auto& pass : passes)
		{
			context.passId = pass.taskName;

			//VerboseLog("Pass: [{}] running for frame {}", pass.taskName, frameInfo.frameIndex);
			pass.execute(buffer, frameInfo, context);
		}

		core->GetFramebufferCache()->EndPass(buffer);

		vkEndCommandBuffer(buffer);

		core->EndFrame(frameInfo);
	}

	void Rendergraph::Rebuild()
	{
		buffers.clear();
		buffers.resize(core->GetSwapchain()->getFramesInFlight());

		VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
		commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocInfo.commandPool = pool;
		commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(buffers.size());

		auto success = vkAllocateCommandBuffers(*core->GetDevice(), &commandBufferAllocInfo, buffers.data());
		Assert(success == VK_SUCCESS, "Failed to allocate command buffers");
		depthImage = nullptr;

		depthImage = new Image(core->GetDevice(), core->GetSwapchain()->getExtent(), VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
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

			if(pass.initialisation != nullptr) pass.initialisation(passResources);
		};

		for (auto& pass : passes) { ProcessPass(pass); }
		for (auto& pass : uniquePasses) { ProcessPass(pass); }
	}

	void Rendergraph::validateGraph() { }

	void Rendergraph::mergePasses() { }

	void Rendergraph::buildTransients() { }

	void Rendergraph::buildBarriers() { }
}
