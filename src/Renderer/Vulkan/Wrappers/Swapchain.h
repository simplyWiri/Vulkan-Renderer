#pragma once
#include "vulkan.h"
#include <vector>
#include <algorithm>

namespace Renderer
{
	struct Context;
	class Window;

	class Swapchain
	{
	public:
		Context* context;
		Window* window;

		VkSwapchainKHR	swapchain = VK_NULL_HANDLE;
		VkFormat		format;
		VkExtent2D		extent;
		VkPresentModeKHR presentMode;
		std::vector<VkImage> images;
		std::vector<VkImageView> imageViews;

	private:
		int width = -1, height = -1;

	public:
		bool buildSwapchain(Context* content, Window* window);
		bool createSwapchainImages();
		bool recreateSwapchain(int w, int h);

		inline void setWidth(int w) { width = w; }
		inline void setHeight(int h) { height = h; }
		inline VkSwapchainKHR* getSwapchain() { return &swapchain; }
		inline const VkFormat& getFormat() const { return format; }
		inline const VkExtent2D& getExtent() const { return extent; }
		inline const VkPresentModeKHR& getPresentMode() const { return presentMode; }

		inline std::vector<VkImage>& getImages() { return images; }
		inline std::vector<VkImageView>& getImageViews() { return imageViews; }

		inline VkImage& getImage(int index) { return images[index]; }
		inline VkImageView& getImageView(int index) { return imageViews[index]; }

		inline uint32_t getSize() const { return static_cast<uint32_t>(images.size()); }
		inline Window* getWindow() { return window; }

		void cleanupSwapchain();

	private:
		VkSurfaceFormatKHR retrieveFormat();
		VkPresentModeKHR retrievePresentMode();
		VkExtent2D retrieveExtent();
	};
}