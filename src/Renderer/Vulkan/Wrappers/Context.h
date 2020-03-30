#pragma once
#include "vulkan.h"
#include <vector>



enum class PhysicalDevicePreference {
	None, Discrete, Integrated, Virtual, CPU, Multi
};



struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily = -1;
	int transferFamily = -1;
	int computeFamily = -1;

	bool isComplete() {
		return graphicsFamily >= 0 && presentFamily >= 0;
	}
};
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct Window;

struct Context {
public:
	// Initialisation settings
	struct ContextSettings {
		// Instance Settings
		uint32_t apiMajor = 1, apiMinor = 2;
		std::vector<const char*> instanceExtensions = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
		std::vector<const char*> validationLayers = { "VK_LAYER_LUNARG_standard_validation", "VK_LAYER_LUNARG_monitor" };
		bool enableValidationLayers = true;

		// Physical Device Settings
		PhysicalDevicePreference pref = PhysicalDevicePreference::Discrete;
		std::vector<const char*> physExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		bool distinctQueues = false;
	} settings;
	// Physical Device Wrapper
	struct PhysicalDevice {
		VkPhysicalDevice	physDevice;
		QueueFamilyIndices	indices;
	} gpu;
	// Swapchain Wrapper
	struct Swapchain {
		VkSwapchainKHR swapchain;
		SwapChainSupportDetails details;
	} swapchain;
	// Device holder
	struct Device {
		VkDevice device;
		VkQueue graphicsQueue, computeQueue, presentQueue, transferQueue;
	} device;

	VkInstance			instance;


	VkDebugUtilsMessengerEXT debugMessenger;

	void cleanup() {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, nullptr);
		}

		vkDestroyInstance(instance, nullptr);
	}
public:
	inline VkDevice getDevice() const { return device.device; }
	inline VkPhysicalDevice getPhysicalDevice() const { return gpu.physDevice; }
	inline VkInstance getInstance() const { return instance; }
	inline VkSwapchainKHR getSwapchainKHR() const { return swapchain.swapchain; }
	inline VkQueue getGraphicsQueue() const { return device.graphicsQueue; }
	inline VkQueue getTransferQueue() const { return device.transferQueue; }
	inline VkQueue getComputeQueue() const { return device.computeQueue; }
	inline VkQueue getPresentQueue() const { return device.presentQueue; }

private:

	// Device
	// Physical Device
	// Instance
	// Validation Layers
};

namespace Wrappers {
	bool buildInstance(Context* context);
	bool buildDevice(Context* context);
	bool pickPhysicalDevice(Context* context, Window* window);
}
