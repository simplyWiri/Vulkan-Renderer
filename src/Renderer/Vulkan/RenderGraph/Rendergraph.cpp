#include "Rendergraph.h"

#include "../Core.h"

namespace Renderer
{

	Rendergraph::Rendergraph(Core* core)
	{
		this->core = core;
		framebufferCache.buildCache(core->GetDevice()->getDevice(), core->GetSwapchain()->getFramesInFlight());
		renderCache.buildCache(core->GetDevice()->getDevice());
		graphicsPipelineCache.buildCache(core->GetDevice()->getDevice());
		descriptorSetCache.buildCache(core->GetDevice()->getDevice(), core->GetAllocator(), core->GetSwapchain()->getFramesInFlight());

		buffers.resize(core->GetSwapchain()->getFramesInFlight());

		//renderCache.buildCache(core->GetDevice()->getDevice());
		//RenderpassKey key = RenderpassKey(
		//	{ {core->GetSwapchain()->getFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR} }
		//, {});

		//renderCache.add(key);

		//graphicsPipelineCache.buildCache(core->GetDevice()->getDevice());

		//VerboseLog("Baking pipeline Key");
		//auto gpKey = graphicsPipelineCache.bakeKey(
		//	renderCache[key]->getHandle(),
		//	core->GetSwapchain()->getExtent(),
		//	DepthSettings::Disabled(),
		//	{ BlendSettings::Add() },
		//	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		//	{
		//		shaderManager.defaultVertex(),
		//		shaderManager.defaultFragment(),
		//	});

		//framebufferCache.buildCache()

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
		VkCommandBuffer buffer = buffers[currentOffset++ % core->GetSwapchain()->getFramesInFlight()];

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(buffer, &beginInfo);
		
		core->BeginFrame(buffer, frameInfo);

		framebufferCache.BeginPass(
			buffer,
			frameInfo.offset,
			{ frameInfo.imageView },
			renderCache.get(RenderpassKey({ {core->GetSwapchain()->getFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR} }, {})),
			core->GetSwapchain()->getExtent());

		GraphContext context;
		context.extent = core->GetSwapchain()->getExtent();
		context.renderpass = renderCache.get(RenderpassKey({ {core->GetSwapchain()->getFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR} }, {}));
		context.graph = this;

		for (auto& pass : passes)
		{
			context.passId = pass.taskName;

			pass.execute(buffer, frameInfo, context);
		}

		framebufferCache.EndPass(buffer);

		vkEndCommandBuffer(buffer);

		//vkDeviceWaitIdle(*core->GetDevice());

		//for (auto& pass : uniquePasses) // these passes will have their own renderpasses, thus we don't encapsulate them
		//{
		//	GraphContext context;
		//	context.passId = pass.taskName;
		//	context.extent = core->GetSwapchain()->getExtent();

		//	pass.execute(buffer, frameInfo, context);
		//}

		core->EndFrame(frameInfo);
	}

	void Rendergraph::AddPass(PassDesc passDesc)
	{
		if (passDesc.target.has_value() || passDesc.extent.has_value()) uniquePasses.emplace_back(passDesc);
		else passes.emplace_back(passDesc);
	}

	void Rendergraph::extractGraphInformation()
	{
		//auto ProcessPass = [&](PassDesc& pass)
		//{
		//	Tether passResources;
		//	passResources.passId = pass.taskName + "-"; // looks like "passname-resource"
		//	passResources.shaderManager = &shaderManager;

		//	pass.initialisation(passResources);

		//	/* Extract all of our buffers*/
		//	for (auto& bufferResource : passResources.buffers)
		//	{
		//		processBuffer(std::get<0>(bufferResource), std::get<1>(bufferResource));
		//	}

		//	///* Extract all of our images*/
		//	//for (auto& imageResource : passResources.images)
		//	//{
		//	//	processImage(std::get<0>(imageResource), std::get<1>(imageResource));
		//	//}
		//};

		//for (auto& pass : passes)
		//{
		//	ProcessPass(pass);
		//}
		//for (auto& pass : uniquePasses)
		//{
		//	ProcessPass(pass);
		//}
	}

	void Rendergraph::validateGraph()
	{
	}

	void Rendergraph::mergePasses()
	{
	}

	void Rendergraph::buildTransients()
	{
	}

	void Rendergraph::buildBarriers()
	{
	}

	void Rendergraph::processBuffer(const std::string& resName, VkBufferUsageFlags usage)
	{
		//VmaMemoryUsage memUsage;

		//switch (usage)
		//{
		//case VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT:
		//	memUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		//	break;
		//case VK_BUFFER_USAGE_INDEX_BUFFER_BIT:
		//	memUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		//	break;
		//case VK_BUFFER_USAGE_VERTEX_BUFFER_BIT:
		//	memUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		//	break;
		//default:
		//	LogError("Failed to parse the type of VkBufferUsageFlags");
		//	break;
		//}

		//buffers.emplace(resName, new Buffer(core->GetAllocator(), VkDeviceSize(1024), usage, memUsage));
	}

	void Rendergraph::processImage(std::string resName, VkImageUsageFlags usage)
	{
	}
}
