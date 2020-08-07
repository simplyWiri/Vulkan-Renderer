#include "Core.h"

namespace Renderer
{
	Core::Core(int x, int y, const char* name) { TempLogger::Init(); }

	bool Core::Initialise()
	{
		/*
			This will initialise the (mostly) static states of the renderer, customisation can come later.
		*/
		swapchain.Initialise(device.getDevice(), device.getInstance(), device.getPhysicalDevice());

		swapchain.BuildWindow(640, 400, "Vulk");

		device.BuildInstance(true);
		swapchain.BuildSurface();
		device.PickPhysicalDevice(swapchain.getSurface());
		device.BuildLogicalDevice(swapchain.getPresentQueue());
		device.BuildAllocator();

		swapchain.BuildSwapchain();
		swapchain.InitialiseSyncObjects();

		framebufferCache.buildCache(device.getDevice(), swapchain.getFramesInFlight());
		renderpassCache.buildCache(device.getDevice());
		graphicsPipelineCache.buildCache(device.getDevice());
		descriptorCache.buildCache(device.getDevice(), GetAllocator(), swapchain.getFramesInFlight());

		VkCommandPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolCreateInfo.queueFamilyIndex = device.getIndices()->graphicsFamily;
		poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		auto success = vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool);
		Assert(success == VK_SUCCESS, "Failed to create command pool");

		rendergraph = std::make_unique<Rendergraph>(this);

		return true;
	}

	bool Core::Run()
	{
		if (glfwWindowShouldClose(swapchain.getWindow())) return false;

		glfwPollEvents();

		return true;
	}

	void Core::windowResize()
	{
		int width = 0, height = 0;
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(swapchain.getWindow(), &width, &height);
			glfwWaitEvents();
		}
		vkDeviceWaitIdle(device);
		vkQueueWaitIdle(device.queues.graphics);

		swapchain.BuildSwapchain();

		rendergraph->Rebuild();
	}

	void Core::copyBufferToImage(Buffer buffer, Image image)
	{
		VkCommandBuffer commandBuffer = getCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = image.getExtent3D();

		vkCmdCopyBufferToImage(commandBuffer, buffer.getBuffer(), image.getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		vkEndCommandBuffer(commandBuffer);


		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(device.queues.transfer, 1, &submitInfo, nullptr);
		vkQueueWaitIdle(device.queues.transfer);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	void Core::setImageLayout(VkCommandBuffer buffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
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

	VkCommandBuffer Core::getCommandBuffer(VkCommandBufferLevel level, bool begin)
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

	void Core::flushCommandBuffer(VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(device.queues.graphics, 1, &submitInfo, nullptr);
		vkQueueWaitIdle(device.queues.graphics);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	void Core::BeginFrame(VkCommandBuffer& buffer, FrameInfo& info) { info = swapchain.BeginFrame(buffer); }

	void Core::EndFrame(FrameInfo info)
	{
		auto result = swapchain.EndFrame(info, device.queues.graphics);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) windowResize();
	}

	Core::~Core()
	{
		vkDestroyCommandPool(device, commandPool, nullptr);
		framebufferCache.clearCache();
		graphicsPipelineCache.clearCache();
		renderpassCache.clearCache();
		descriptorCache.clearCache();
	}
}
