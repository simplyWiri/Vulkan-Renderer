#pragma once
#include "vulkan.h"
#include <vector>
#include <algorithm>



namespace Renderer {

	struct Context;
	struct Window;

	struct Swapchain {
	public:
		Context* context;
		Window* window;

		VkSwapchainKHR	swapchain = VK_NULL_HANDLE;
		VkFormat		format;
		VkExtent2D		extent;
		VkPresentModeKHR presentMode;
		std::vector<VkImage> images;
		std::vector<VkImageView> imageViews;

		bool recreateSwapchain(int w, int h);

		inline void setWidth(int w) { width = w; }
		inline void setHeight(int h) { height = h; }
		inline uint32_t getSize() { return static_cast<uint32_t>(images.size()); }

		void cleanupSwapchain();
	private:
		int width = -1, height = -1;
	};

	namespace Wrappers {
		// Initialisation, our swapchain will keep reference to both structs
		bool buildSwapchain(Swapchain* swapchain, Context* context, Window* window);

		bool createSwapchainImages(Swapchain* swapchain);
	}

}