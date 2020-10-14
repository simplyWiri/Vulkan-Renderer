#pragma once
#include "vulkan.h"
#include <vector>

namespace Renderer
{
	// Debug Layer related functions
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	struct QueueFamilyIndices
	{
		int graphicsFamily = -1;
		int presentFamily = -1;
		int transferFamily = -1;
		int computeFamily = -1;

		bool isComplete() const { return graphicsFamily >= 0 && presentFamily >= 0; }
	};

	class Device
	{
	public:
		~Device();

		struct
		{
			VkQueue graphics;
			VkQueue compute;
			VkQueue transfer;
		} queues;

		struct SwapInformation
		{
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		} swapDetails;

		struct
		{
			std::vector<const char*> physExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
			std::vector<const char*> instanceExtensions = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
			const std::vector<const char*> validationLayers = {
#ifdef DEBUG
				"VK_LAYER_KHRONOS_validation"
#endif
			};
		} extensions;

		operator VkDevice() { return device; }

	private:
		VkDevice device;
		VkPhysicalDevice physDevice;
		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		bool debug = false;
		QueueFamilyIndices indices;

		// physical device details
		VkPhysicalDeviceFeatures features;
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceMemoryProperties memProperties;
	public:
		VkDevice* GetDevice() { return &device; }
		VkPhysicalDevice* GetPhysicalDevice() { return &physDevice; }
		VkInstance* GetInstance() { return &instance; }
		QueueFamilyIndices* GetIndices() { return &indices; }

		VkPhysicalDeviceFeatures GetPhysicalDeviceFeatures() { return features; }
		VkPhysicalDeviceProperties GetPhysicalDeviceProperties() { return properties; }
		VkPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties() { return memProperties; }


		void BuildInstance(bool debugLayers);
		void PickPhysicalDevice(VkSurfaceKHR* surface);
		void BuildLogicalDevice(VkQueue* presentQueue);
		
	private:
		// Physical Device related functions
		QueueFamilyIndices GetIndices(VkPhysicalDevice physDevice, VkSurfaceKHR* surface);

		bool CheckDeviceExtensionSupport(VkPhysicalDevice physDevice);
		void CheckSwapChainSupport(VkPhysicalDevice physDevice, VkSurfaceKHR* surface, VkSurfaceCapabilitiesKHR* capabilities, std::vector<VkSurfaceFormatKHR>& formats, std::vector<VkPresentModeKHR>& presentModes);

		bool IsDeviceSuitable(VkPhysicalDevice physDevice, VkSurfaceKHR* surface, VkSurfaceCapabilitiesKHR* capabilities, std::vector<VkSurfaceFormatKHR>& formats, std::vector<VkPresentModeKHR>& presentModes);
	};
}
