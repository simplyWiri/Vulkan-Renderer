#pragma once
#include "vulkan.h"
#include <vector>
#include <memory>
#include <chrono>

#include "../Memory/Image.h"

struct GLFWwindow;

namespace Renderer
{
	class Core;
	class PassDesc;
	
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
		friend void DrawDebugVisualisations(Core* core, FrameInfo& frameInfo, const std::vector<std::unique_ptr<PassDesc>>& passes);
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
		std::vector<Memory::Image*> images;
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
		std::vector<Memory::Image*>& GetImages() { return images; }
		VkExtent2D GetExtent() { return extent; }
		uint32_t GetIndex() { return currentIndex; }
		uint32_t GetFramesInFlight() { return framesInFlight; }

	public:

		void Initialise(VkDevice* device, VkInstance* instance, VkPhysicalDevice* physDevice);
		void BuildWindow(int width, int height, const char* title);
		void BuildSurface();
		void BuildSwapchain(bool vsync = false);
		void BuildSyncObjects();

		FrameInfo BeginFrame(VkCommandBuffer buffer);
		VkResult EndFrame(FrameInfo& info, VkQueue graphicsQueue);

	private:
		void CheckSwapChainSupport(VkSurfaceCapabilitiesKHR* capabilities, std::vector<VkSurfaceFormatKHR>& formats, std::vector<VkPresentModeKHR>& presentModes);
	};
}
