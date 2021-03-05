#include <memory>
#include <chrono>

#include "tracy/Tracy.hpp"
#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
#include "glfw/include/GLFW/glfw3.h"

#include "Swapchain.h"
#include "Utils/Logging.h"

namespace Renderer
{
	Swapchain::~Swapchain()
	{
		for (auto image : views) vkDestroyImageView(device, image, nullptr);

		vkDestroySwapchainKHR(device, swapchain, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	Swapchain::FrameResources::~FrameResources()
	{
		if (device != nullptr)
		{
			vkDestroySemaphore(device, imageAcquired, nullptr);
			vkDestroySemaphore(device, renderFinished, nullptr);
			vkDestroyFence(device, inFlightFence, nullptr);
		}
	}

	void Swapchain::Initialise(VkDevice device, VkInstance instance, VkPhysicalDevice physDevice)
	{
		this->device = device;
		this->instance = instance;
		this->physDevice = physDevice;
	}

	void Swapchain::BuildWindow(int width, int height, const char* title)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(width, height, title, nullptr, nullptr);
		Assert(window != nullptr, "Failed to create window");
	}

	void Swapchain::BuildSurface()
	{
		auto success = glfwCreateWindowSurface(instance, window, nullptr, &surface);
		Assert(success == VK_SUCCESS, "Failed to initialise window surface");
	}

	void Swapchain::BuildSwapchain(bool vsync)
	{
		VkSwapchainKHR oldSwapchain = swapchain;

		VkSurfaceCapabilitiesKHR surfCaps;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
		CheckSwapChainSupport(&surfCaps, formats, presentModes);

		if (surfCaps.currentExtent.width == static_cast<uint32_t>(-1))
		{
			extent.width = width;
			extent.height = height;
		}
		else { extent = surfCaps.currentExtent; }

		VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

		if (!vsync)
		{
			for (size_t i = 0; i < presentModes.size(); i++)
			{
				if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}
				if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			}
		}

		uint32_t swapImagesCount = surfCaps.minImageCount + 1;
		if ((surfCaps.maxImageCount > 0) && (swapImagesCount > surfCaps.maxImageCount)) swapImagesCount = surfCaps.maxImageCount;

		VkSurfaceTransformFlagsKHR preTransform;
		if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		else preTransform = surfCaps.currentTransform;

		VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = { VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR, };

		for (auto& compositeAlphaFlag : compositeAlphaFlags)
		{
			if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag)
			{
				compositeAlpha = compositeAlphaFlag;
				break;
			}
		}

		color = {};
		if (color.format == VK_FORMAT_UNDEFINED)
		{
			if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
			{
				color.format = VK_FORMAT_B8G8R8A8_UNORM;
				color.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			}
			for (const auto& format : formats)
			{
				if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				{
					color.format = format.format;
					color.colorSpace = format.colorSpace;
					break;
				}
			}
		}

		VkSwapchainCreateInfoKHR swapchainCI = {};
		swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCI.pNext = nullptr;
		swapchainCI.surface = surface;
		swapchainCI.minImageCount = swapImagesCount;
		swapchainCI.imageFormat = color.format;
		swapchainCI.imageColorSpace = color.colorSpace;
		swapchainCI.imageExtent = extent;
		swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCI.preTransform = static_cast<VkSurfaceTransformFlagBitsKHR>(preTransform);
		swapchainCI.imageArrayLayers = 1;
		swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCI.queueFamilyIndexCount = 0;
		swapchainCI.pQueueFamilyIndices = nullptr;
		swapchainCI.presentMode = swapchainPresentMode;
		swapchainCI.oldSwapchain = oldSwapchain;
		swapchainCI.clipped = VK_TRUE;
		swapchainCI.compositeAlpha = compositeAlpha;

		if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		auto success = vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain);
		Assert(success == VK_SUCCESS, "Failed to create swapchain");

		if (oldSwapchain != nullptr)
		{
			for (auto image : views) vkDestroyImageView(device, image, nullptr);

			vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
		}

		vkGetSwapchainImagesKHR(device, swapchain, &framesInFlight, nullptr);
		images.resize(framesInFlight);
		vkGetSwapchainImagesKHR(device, swapchain, &framesInFlight, images.data());

		// Get the swap chain buffers containing the image and imageview
		views.resize(framesInFlight);
		for (uint32_t i = 0; i < framesInFlight; i++)
		{
			VkImageViewCreateInfo colorAttachmentView = {};
			colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			colorAttachmentView.pNext = nullptr;
			colorAttachmentView.format = color.format;
			colorAttachmentView.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			colorAttachmentView.subresourceRange.baseMipLevel = 0;
			colorAttachmentView.subresourceRange.levelCount = 1;
			colorAttachmentView.subresourceRange.baseArrayLayer = 0;
			colorAttachmentView.subresourceRange.layerCount = 1;
			colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
			colorAttachmentView.flags = 0;
			colorAttachmentView.image = images[i];

			vkCreateImageView(device, &colorAttachmentView, nullptr, &views[i]);
		}
	}

	void Swapchain::BuildSyncObjects()
	{
		frames.resize(framesInFlight);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t i = 0; i < framesInFlight; i++)
		{
			auto& frame = frames[i];
			frame = {};
			frame.device = device;
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame.imageAcquired);
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame.renderFinished);
			vkCreateFence(device, &fenceInfo, nullptr, &frame.inFlightFence);
		}
	}

	FrameInfo Swapchain::BeginFrame(VkCommandBuffer buffer, const FrameInfo& prevInfo)
	{
		auto& curFrame = frames[currentIndex];
		curFrame.buffer = buffer;

		VkFence waitFences[] = { curFrame.inFlightFence };
		vkWaitForFences(device, 1, waitFences, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &curFrame.inFlightFence);

		uint32_t imageIndex = 0;
		vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, curFrame.imageAcquired, nullptr, &imageIndex);

		FrameInfo info = {};

		info.time = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
		
		if (info.time - prevFpsSample > 1000)
		{
			fps = (frameCount + 1) - prevFrames;
			prevFrames = frameCount + 1;
			prevFpsSample = info.time;
		}

		info.delta = (info.time - prevInfo.time)/1000.0;
		info.fps = fps;
		info.frameIndex = frameCount++;
		info.offset = currentIndex;

		info.imageIndex = imageIndex;
		info.imageView = views[imageIndex];

		return info;
	}

	VkResult Swapchain::EndFrame(FrameInfo& info, VkQueue graphicsQueue)
	{
		auto& frame = frames[currentIndex];

		VkSemaphore waitSemaphores[] = { frame.imageAcquired };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore signalSemaphores[] = { frame.renderFinished };

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &frame.buffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		VkResult success;
		
		{
			ZoneScopedNC("Queue Submit", tracy::Color::Red)
			success = vkQueueSubmit(graphicsQueue, 1, &submitInfo, frame.inFlightFence);
			Assert(success == VK_SUCCESS, "Failed to submit queue");
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &frames[currentIndex].renderFinished;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain;
		presentInfo.pImageIndices = &info.imageIndex;

		{
			ZoneScopedNC("Queue Present", tracy::Color::Red)
			success = vkQueuePresentKHR(presentQueue, &presentInfo);
		}

		++currentIndex %= framesInFlight;

		return success;
	}

	void Swapchain::CheckSwapChainSupport(VkSurfaceCapabilitiesKHR* capabilities, std::vector<VkSurfaceFormatKHR>& formats, std::vector<VkPresentModeKHR>& presentModes)
	{
		// load device SwapChain capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, capabilities);

		// load device SwapChain formats
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, formats.data());
		}

		// load device SwapChain presentcount
		uint32_t presentCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentCount, nullptr);

		if (presentCount != 0)
		{
			presentModes.resize(presentCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentCount, presentModes.data());
		}
	}
}
