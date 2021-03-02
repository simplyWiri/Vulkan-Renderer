#pragma once
#include <vector>

#include "volk/volk.h"

struct GLFWwindow;

namespace Renderer
{
	namespace Memory {
		class Allocator;
	}

	class Core;
	struct FrameInfo;
	
	struct FrameInfo
	{
		uint32_t fps;
		uint32_t offset;
		uint32_t frameIndex;
		long long time;
		double delta;
		uint32_t imageIndex;
		VkImageView imageView;
	};

	class Swapchain
	{
	public:
		~Swapchain();

	private:

		struct FrameResources
		{
			VkSemaphore imageAcquired;
			VkSemaphore renderFinished;
			VkFence inFlightFence;

			VkCommandBuffer buffer;
			VkDevice* device;

			~FrameResources();
		};

		GLFWwindow* window;
		VkSurfaceKHR surface;
		VkSwapchainKHR swapchain = nullptr;
		
		std::vector<VkImage> images;
		std::vector<VkImageView> views;
		std::vector<FrameResources> frames;
		VkQueue presentQueue;
		VkQueue graphicsQueue;

		VkSurfaceFormatKHR color;
		VkExtent2D extent;

		// info
		uint32_t fps = 0;
		uint32_t prevFrames = 0;
		long long prevFpsSample = 0;
		long long prevTime = 0;
		uint32_t frameCount = 0;
		uint32_t currentIndex = 0;
		uint32_t framesInFlight = 3;

		int width = 640;
		int height = 400;

		VkDevice* device;
		VkInstance* instance;
		VkPhysicalDevice* physDevice;

	public:
		operator VkSwapchainKHR() { return swapchain; }
		VkFormat& GetFormat() { return color.format; }
		GLFWwindow* GetWindow() { return window; }
		VkSurfaceKHR* GetSurface() { return &surface; }
		VkSwapchainKHR* GetSwapchain() { return &swapchain; }
		VkQueue* GetPresentQueue() { return &presentQueue; }
		std::vector<VkImageView>& GetImageViews() { return views; }
		VkExtent2D GetExtent() const { return extent; }
		uint32_t GetIndex() const { return currentIndex; }
		uint32_t GetFramesInFlight() const { return framesInFlight; }

	public:

		void Initialise(VkDevice* device, VkInstance* instance, VkPhysicalDevice* physDevice);
		void BuildWindow(int width, int height, const char* title);
		void BuildSurface();
		void BuildSwapchain(bool vsync = false);
		void BuildSyncObjects();

		FrameInfo BeginFrame(VkCommandBuffer buffer, const FrameInfo& prevInfo);
		VkResult EndFrame(FrameInfo& info, VkQueue graphicsQueue);

	private:
		void CheckSwapChainSupport(VkSurfaceCapabilitiesKHR* capabilities, std::vector<VkSurfaceFormatKHR>& formats, std::vector<VkPresentModeKHR>& presentModes);
	};
}
