#pragma once
#include "Device.h"
#include "../../../Utils/Logging.h"
#include "vulkan.h"
#include <vector>
#include <memory>
#include "glfw3.h"

namespace Renderer
{
	struct FrameInfo
	{
		uint32_t frameIndex;
		float time;
		float delta;
	};

	class Swapchain
	{
	public:
		~Swapchain()
		{
			for (auto image : views) vkDestroyImageView(*device, image, nullptr);

			vkDestroySwapchainKHR(*device, swapchain, nullptr);
			vkDestroySurfaceKHR(*instance, surface, nullptr);
			glfwDestroyWindow(window);
			glfwTerminate();
		}

	public:

	private:

		struct FrameResources
		{
			VkSemaphore imageAcquired;
			VkSemaphore renderFinished;
			VkFence inFlightFence;

			VkCommandBuffer buffer;
		};

		/*
			Resources held by this struct
		*/
		GLFWwindow* window;
		VkSurfaceKHR surface;
		VkSwapchainKHR swapchain = VK_NULL_HANDLE;
		std::vector<VkImage> images;
		std::vector<VkImageView> views;
		std::vector<FrameResources> frames;
		VkQueue presentQueue;

		VkSurfaceFormatKHR color;
		VkExtent2D extent;

		// info
		uint32_t currentIndex;
		uint32_t framesInFlight;
		int width = 640;
		int height;
		const char* title;

		VkDevice* device;
		VkInstance* instance;
		VkPhysicalDevice* physDevice;

	public:
		operator VkSwapchainKHR() { return swapchain; }
		VkFormat& getFormat() { return color.format; }
		GLFWwindow* getWindow() { return window; }
		VkSurfaceKHR* getSurface() { return &surface; }
		VkSwapchainKHR* getSwapchain() { return &swapchain; }
		VkQueue* getPresentQueue() { return &presentQueue; }
		std::vector<VkImageView>& getImageViews() { return views; }
		VkExtent2D getExtent() { return extent; }

		void Initialise(VkDevice* device, VkInstance* instance, VkPhysicalDevice* physDevice)
		{
			this->device = device;
			this->instance = instance;
			this->physDevice = physDevice;
		}

		void BuildWindow() { BuildWindow(width, height, title); }

		void BuildWindow(int width, int height, const char* title)
		{
			glfwInit();
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

			window = glfwCreateWindow(width, height, title, nullptr, nullptr);
			Assert(window != nullptr, "Failed to create window");
		}

		void BuildSurface()
		{
			auto success = glfwCreateWindowSurface(*instance, window, nullptr, &surface);
			Assert(success == VK_SUCCESS, "Failed to initialise window surface");
		}

		void BuildSwapchain(bool vsync = false)
		{
			VkSwapchainKHR oldSwapchain = swapchain;

			VkSurfaceCapabilitiesKHR surfCaps;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
			checkSwapChainSupport(&surfCaps, formats, presentModes);

			if (surfCaps.currentExtent.width == (uint32_t)-1)
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
					if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (presentModes[i] ==
						VK_PRESENT_MODE_IMMEDIATE_KHR)) swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
				}
			}

			uint32_t swapImagesCount = surfCaps.minImageCount + 1;
			if ((surfCaps.maxImageCount > 0) && (swapImagesCount > surfCaps.maxImageCount)) swapImagesCount = surfCaps.
				maxImageCount;

			VkSurfaceTransformFlagsKHR preTransform;
			if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) preTransform =
				VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			else preTransform = surfCaps.currentTransform;

			VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

			std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
				VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
				VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
				VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
				VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
			};

			for (auto& compositeAlphaFlag : compositeAlphaFlags)
			{
				if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag)
				{
					compositeAlpha = compositeAlphaFlag;
					break;
				};
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
					if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace ==
						VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
					{
						color.format = format.format;
						color.colorSpace = format.colorSpace;
						break;
					}
				}
			}

			VkSwapchainCreateInfoKHR swapchainCI = {};
			swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapchainCI.pNext = NULL;
			swapchainCI.surface = surface;
			swapchainCI.minImageCount = swapImagesCount;
			swapchainCI.imageFormat = color.format;
			swapchainCI.imageColorSpace = color.colorSpace;
			swapchainCI.imageExtent = extent;
			swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
			swapchainCI.imageArrayLayers = 1;
			swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapchainCI.queueFamilyIndexCount = 0;
			swapchainCI.pQueueFamilyIndices = NULL;
			swapchainCI.presentMode = swapchainPresentMode;
			swapchainCI.oldSwapchain = oldSwapchain;
			swapchainCI.clipped = VK_TRUE;
			swapchainCI.compositeAlpha = compositeAlpha;

			if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) swapchainCI.imageUsage |=
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) swapchainCI.imageUsage |=
				VK_IMAGE_USAGE_TRANSFER_DST_BIT;

			auto success = vkCreateSwapchainKHR(*device, &swapchainCI, nullptr, &swapchain);
			Assert(success == VK_SUCCESS, "Failed to create swapchain");

			if (oldSwapchain != VK_NULL_HANDLE)
			{
				for (auto image : views) vkDestroyImageView(*device, image, nullptr);

				vkDestroySwapchainKHR(*device, oldSwapchain, nullptr);
			}

			vkGetSwapchainImagesKHR(*device, swapchain, &swapImagesCount, nullptr);
			images.resize(swapImagesCount);
			vkGetSwapchainImagesKHR(*device, swapchain, &swapImagesCount, images.data());

			// Get the swap chain buffers containing the image and imageview
			views.resize(swapImagesCount);
			for (uint32_t i = 0; i < swapImagesCount; i++)
			{
				VkImageViewCreateInfo colorAttachmentView = {};
				colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				colorAttachmentView.pNext = NULL;
				colorAttachmentView.format = color.format;
				colorAttachmentView.components = {
					VK_COMPONENT_SWIZZLE_R,
					VK_COMPONENT_SWIZZLE_G,
					VK_COMPONENT_SWIZZLE_B,
					VK_COMPONENT_SWIZZLE_A
				};
				colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				colorAttachmentView.subresourceRange.baseMipLevel = 0;
				colorAttachmentView.subresourceRange.levelCount = 1;
				colorAttachmentView.subresourceRange.baseArrayLayer = 0;
				colorAttachmentView.subresourceRange.layerCount = 1;
				colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
				colorAttachmentView.flags = 0;
				colorAttachmentView.image = images[i];

				vkCreateImageView(*device, &colorAttachmentView, nullptr, &views[i]);
			}
		}

		// this will begin the frame, waiting for relevant frame resources
		FrameInfo BeginFrame() { auto& curFrame = frames[currentIndex]; }

		// this will end the frame, submitting to the presentation queue
		VkResult EndFrame() { ++currentIndex %= framesInFlight; }

	private:
		void checkSwapChainSupport(VkSurfaceCapabilitiesKHR* capabilities, std::vector<VkSurfaceFormatKHR>& formats,
		                           std::vector<VkPresentModeKHR>& presentModes)
		{
			// load device SwapChain capabilities
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*physDevice, surface, capabilities);

			// load device SwapChain formats
			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(*physDevice, surface, &formatCount, nullptr);
			if (formatCount != 0)
			{
				formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(*physDevice, surface, &formatCount, formats.data());
			}

			// load device SwapChain presentcount
			uint32_t presentCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(*physDevice, surface, &presentCount, nullptr);

			if (presentCount != 0)
			{
				presentModes.resize(presentCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(*physDevice, surface, &presentCount, presentModes.data());
			}
		}
	};
}
