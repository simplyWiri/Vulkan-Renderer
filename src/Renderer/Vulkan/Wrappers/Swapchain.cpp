#pragma once
#include "../../../Utils/Logging.h"
#undef max
#undef min
#include "Swapchain.h"
#include "Context.h"
#include "Window.h"

namespace Renderer
{
	VkSurfaceFormatKHR Swapchain::retrieveFormat()
	{
		if (context->swapDetails.formats.size() == 1 && context->swapDetails.formats[0].format == VK_FORMAT_UNDEFINED)
			return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

		for (const auto& format : context->swapDetails.formats) {
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return format;
		}

		return context->swapDetails.formats[0];
	}
	VkPresentModeKHR Swapchain::retrievePresentMode()
	{
		VkPresentModeKHR optimalMode = VK_PRESENT_MODE_FIFO_KHR;

		for (const auto& presentMode : context->swapDetails.presentModes) {
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				return presentMode;
			if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				optimalMode = presentMode;
		}

		return optimalMode;
	}
	VkExtent2D Swapchain::retrieveExtent()
	{
		auto capabilities = context->swapDetails.capabilities;
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return capabilities.currentExtent;

		VkExtent2D curExtent = { static_cast<uint32_t>(window->getWidth()), static_cast<uint32_t>(window->getHeight()) };

		curExtent.width = std::max(capabilities.minImageExtent.width,
			std::min(capabilities.maxImageExtent.width, curExtent.width));

		curExtent.height = std::max(capabilities.minImageExtent.height,
			std::min(capabilities.maxImageExtent.height, curExtent.height));

		return curExtent;
	}

	bool Swapchain::buildSwapchain(Context* context, Window* window)
	{
		setWidth(window->getWidth());
		setHeight(window->getHeight());
		this->context = context;
		this->window = window;

		SwapChainSupportDetails swapDetails = context->swapDetails;
		format = retrieveFormat().format;
		presentMode = retrievePresentMode();
		extent = retrieveExtent();

		uint32_t imageCount = swapDetails.capabilities.minImageCount + 1;
		if (swapDetails.capabilities.maxImageCount > 0 && imageCount > swapDetails.capabilities.maxImageCount) {
			imageCount = swapDetails.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = window->getSurface();
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = format;
		createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t indices[] = { static_cast<uint32_t>(context->gpu.indices.graphicsFamily), static_cast<uint32_t>(context->gpu.indices.presentFamily) };

		if (indices[0] != indices[1]) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = indices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		createInfo.preTransform = swapDetails.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		auto success = vkCreateSwapchainKHR(context->getDevice(), &createInfo, nullptr, &swapchain);
		Assert(success == VK_SUCCESS, "Failed to create swapchain");

		vkGetSwapchainImagesKHR(context->getDevice(), swapchain, &imageCount, nullptr);
		images.resize(imageCount);
		vkGetSwapchainImagesKHR(context->getDevice(), swapchain, &imageCount, images.data());

		return createSwapchainImages();
	}

	bool Swapchain::createSwapchainImages()
	{
		imageViews.resize(images.size());

		for (uint32_t i = 0; i < images.size(); i++) {
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = images[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = format;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(context->getDevice(), &createInfo, nullptr, &imageViews[i]) != VK_SUCCESS)
				return false;
		}
		return true;
	}

	bool Swapchain::recreateSwapchain(int w, int h)
	{
		VkSwapchainKHR newSwapchain;

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = window->getSurface();
		createInfo.minImageCount = static_cast<uint32_t>(images.size()); // no reason for it to change
		VkSurfaceFormatKHR format = retrieveFormat();
		createInfo.imageFormat = format.format;
		createInfo.imageColorSpace = format.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t indices[] = { static_cast<uint32_t>(context->gpu.indices.graphicsFamily), static_cast<uint32_t>(context->gpu.indices.presentFamily) };

		if (indices[0] != indices[1]) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = indices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		createInfo.preTransform = context->swapDetails.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = swapchain;

		if (vkCreateSwapchainKHR(context->getDevice(), &createInfo, nullptr, &newSwapchain) != VK_SUCCESS)
			return false;

		swapchain = VkSwapchainKHR(newSwapchain); // copy constructor the new swapchain into the struct

		return true;
	}

	void Swapchain::cleanupSwapchain()
	{
		for (auto image : imageViews)
			vkDestroyImageView(context->getDevice(), image, nullptr);

		vkDestroySwapchainKHR(context->getDevice(), swapchain, nullptr);
	}
}