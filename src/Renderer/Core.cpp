#include "Core.h"

#include "glfw3.h"
#include "Memory/Allocator.h"

namespace Renderer
{
	Core::Core(Settings settings) : settings(settings) { TempLogger::Init(); }

	bool Core::Initialise()
	{
		/*
			This will initialise the (mostly) static states of the renderer, customisation can come later.
		*/
		swapchain.Initialise(device.GetDevice(), device.GetInstance(), device.GetPhysicalDevice());

		swapchain.BuildWindow(settings.width, settings.height, settings.name);

#ifdef NDEBUG
		device.BuildInstance(false);
#elif DEBUG
		device.BuildInstance(true);
#endif
		swapchain.BuildSurface();
		device.PickPhysicalDevice(swapchain.GetSurface());
		device.BuildLogicalDevice(swapchain.GetPresentQueue());

		swapchain.BuildSwapchain(settings.vsync);
		swapchain.BuildSyncObjects();

		allocator = new Memory::Allocator(GetDevice(), GetSwapchain()->GetFramesInFlight());

		framebufferCache.BuildCache(device.GetDevice(), swapchain.GetFramesInFlight());
		renderpassCache.BuildCache(device.GetDevice());
		graphicsPipelineCache.BuildCache(device.GetDevice());
		descriptorCache.BuildCache(device.GetDevice(), allocator, swapchain.GetFramesInFlight());
		shaderManager = new ShaderManager(device.GetDevice());

		VkCommandPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolCreateInfo.queueFamilyIndex = device.GetIndices()->graphicsFamily;
		poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		auto success = vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool);
		Assert(success == VK_SUCCESS, "Failed to create command pool");

		rendergraph = std::make_unique<Rendergraph>(this);

		return true;
	}

	bool Core::Run()
	{
		if (glfwWindowShouldClose(swapchain.GetWindow())) return false;

		glfwPollEvents();

		return true;
	}

	void Core::windowResize()
	{
		settings.width = 0;
		settings.height = 0;
		while (settings.width == 0 || settings.height == 0)
		{
			glfwGetFramebufferSize(swapchain.GetWindow(), &settings.width, &settings.height);
			glfwWaitEvents();
		}
		vkDeviceWaitIdle(device);
		vkQueueWaitIdle(device.queues.graphics);

		swapchain.BuildSwapchain();

		rendergraph->Rebuild();
	}

	void Core::SetImageLayout(VkCommandBuffer buffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
	{
		// Create an image barrier object
		VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		imageMemoryBarrier.oldLayout = oldImageLayout;
		imageMemoryBarrier.newLayout = newImageLayout;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange = subresourceRange;

		switch (oldImageLayout)
		{
			case VK_IMAGE_LAYOUT_UNDEFINED: imageMemoryBarrier.srcAccessMask = 0;
				break;
			case VK_IMAGE_LAYOUT_PREINITIALIZED: imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;

			default: Assert(false, "Not supported");
				break;
		}

		switch (newImageLayout)
		{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: if (imageMemoryBarrier.srcAccessMask == 0) imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;

			default: Assert(false, "Not supported");
				break;
		}

		// Put barrier inside setup command buffer
		vkCmdPipelineBarrier(buffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}

	VkCommandBuffer Core::GetCommandBuffer(VkCommandBufferLevel level, bool begin)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = level;
		allocInfo.commandBufferCount = static_cast<uint32_t>(1);

		VkCommandBuffer cmdBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer);
		// If requested, also start recording for the new command buffer
		if (begin)
		{
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			vkBeginCommandBuffer(cmdBuffer, &beginInfo);
		}
		return cmdBuffer;
	}

	void Core::FlushCommandBuffer(VkCommandBuffer buffer)
	{
		vkEndCommandBuffer(buffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &buffer;

		vkQueueSubmit(device.queues.graphics, 1, &submitInfo, nullptr);
		vkQueueWaitIdle(device.queues.graphics);

		vkFreeCommandBuffers(device, commandPool, 1, &buffer);
	}

	void Core::BeginFrame(VkCommandBuffer& buffer, FrameInfo& info)
	{
		info = swapchain.BeginFrame(buffer);

		descriptorCache.Tick();
		graphicsPipelineCache.Tick();
		renderpassCache.Tick();
		framebufferCache.Tick();
	}

	void Core::EndFrame(FrameInfo info)
	{
		const auto result = swapchain.EndFrame(info, device.queues.graphics);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) windowResize();
	}

	Core::~Core()
	{
		rendergraph.reset();
		delete allocator;

		vkDestroyCommandPool(device, commandPool, nullptr);
		framebufferCache.ClearCache();
		graphicsPipelineCache.ClearCache();
		renderpassCache.ClearCache();
		descriptorCache.ClearCache();

		delete shaderManager;
	}
}
