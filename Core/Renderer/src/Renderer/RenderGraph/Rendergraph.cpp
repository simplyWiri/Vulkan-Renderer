#include "Rendergraph.h"

#include "../Core.h"
#include "imgui.h"
#include "../Memory/Allocator.h"
#include "../Memory/Image.h"
#include "../Memory/Block.h"
#include "../../Utils/DebugVisualisations.h"

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

		depthImage = core->GetAllocator()->AllocateImage(core->GetSwapchain()->GetExtent(), VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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

		{
#if DEBUG
			ZoneScopedNC("Drawing Debug Visualisations", tracy::Color::Green)
			DrawDebugVisualisations(core, frameInfo, passes);
#else
			ImGui::NewFrame();
#endif

		}

		vkBeginCommandBuffer(buffer, &beginInfo);

		auto renderpass = core->GetRenderpassCache()->Get(RenderpassKey({ { core->GetSwapchain()->GetFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR } }, { depthImage->GetFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR }));

		core->GetFramebufferCache()->BeginPass(buffer, frameInfo.offset, { frameInfo.imageView, depthImage->GetView() }, renderpass, core->GetSwapchain()->GetExtent());

		GraphContext context{this, "", core->GetSwapchain()->GetWindow(), renderpass->GetHandle()};

		{
			ZoneScopedNC("Gathering Draw Commands", tracy::Color::Green)
			
			for (auto& pass : passes)
			{
				context.passId = pass.taskName;
				pass.execute(buffer, frameInfo, context);
			}
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
		depthImage = core->GetAllocator()->AllocateImage(core->GetSwapchain()->GetExtent(), VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	void Rendergraph::AddPass(PassDesc passDesc)
	{
		/*if (passDesc.target.has_value() || passDesc.extent.has_value()) uniquePasses.emplace_back(passDesc);
		*/
		passes.emplace_back(passDesc);
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

}
