#include <set>
#include <string>

#include "glfw/include/GLFW/glfw3.h"

#include "Device.h"
#include "Utils/Logging.h"

namespace Renderer
{
	VkBool32 DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		char ms;
		switch (messageSeverity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: ms = 'V';
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: ms = 'E';
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: ms = 'W';
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: ms = 'I';
				break;
			default: ms = 'U';
				break;
		}

		const char* mt;
		switch (static_cast<int>(messageType))
		{
			case 7: mt = "General | Validation | Performance";
				break;
			case 6: mt = "Validation | Performance";
				break;
			case 5: mt = "General | Performance";
				break;
			case 4: mt = "Performance";
				break;
			case 3: mt = "General | Validation";
				break;
			case 2: mt = "Validation";
				break;
			case 1: mt = "General";
				break;
			default: mt = "Unknown";
				break;
		}

		switch (ms)
		{
			case 'E': LogError("[{0}]: {1}", mt, pCallbackData->pMessage);
				break;
			case 'W': LogWarning("[{0}]: {1}", mt, pCallbackData->pMessage);
				break;
			case 'I': LogInfo("[{0}]: {1}", mt, pCallbackData->pMessage);
				break;
			default: VerboseLog("[{0}]: {1}", mt, pCallbackData->pMessage);
				break;
		}

		if(_strcmpi(mt, "General") == 1) __debugbreak();
		return VK_FALSE;
	}

	Device::~Device()
	{
		vkDestroyDevice(device, nullptr);

		if (debug)
		{
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			if (func != nullptr) { func(instance, debugMessenger, nullptr); }
		}

		vkDestroyInstance(instance, nullptr);
	}

	void Device::BuildInstance(bool debugLayers)
	{
		this->debug = debugLayers;

		{
			// Get our required glfw extensions
			uint32_t glfwExtensionCount = 0;
			const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
			std::vector<const char*> glfwExts(glfwExtensions, glfwExtensions + glfwExtensionCount);
			for (auto extension : glfwExts) extensions.instanceExtensions.emplace_back(extension);
		}

		VkApplicationInfo info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
		info.pApplicationName = "Application Name";
		info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		info.pEngineName = "Engine Name";
		info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		info.apiVersion = VK_MAKE_VERSION(1, 2, 0);

		VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		createInfo.pApplicationInfo = &info;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.instanceExtensions.size());
		createInfo.ppEnabledExtensionNames = extensions.instanceExtensions.data();

		// Enable validation layers if req'd
		VkDebugUtilsMessengerCreateInfoEXT messengerInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
		if (debug)
		{
			messengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			messengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			messengerInfo.pfnUserCallback = DebugCallback;

			createInfo.enabledLayerCount = static_cast<uint32_t>(extensions.validationLayers.size());
			createInfo.ppEnabledLayerNames = extensions.validationLayers.data();
			createInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&messengerInfo);
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		auto volkInit = volkInitialize();
		auto success = vkCreateInstance(&createInfo, nullptr, &instance);
		volkLoadInstance(instance);
		Assert(success == VK_SUCCESS, "Failed to create instance");

		if (debug)
		{
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
			if (func != nullptr) { func(instance, &messengerInfo, nullptr, &debugMessenger); }
			else { LogError("Failed to initialise debug layers"); }
		}
	}

	void Device::PickPhysicalDevice(VkSurfaceKHR* surface)
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0) // No GPU's provide support for vulkan!
		{
			LogError("Failed to find a GPU which supports Vulkan on your system");
			return;
		}

		std::vector<VkPhysicalDevice> physDevices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, physDevices.data());

		for (const auto& physDevice : physDevices)
		{
			if (IsDeviceSuitable(physDevice, surface, &this->swapDetails.capabilities, this->swapDetails.formats, this->swapDetails.presentModes))
			{
				this->physDevice = physDevice;
				vkGetPhysicalDeviceFeatures(physDevice, &features);
				vkGetPhysicalDeviceProperties(physDevice, &properties);
				vkGetPhysicalDeviceMemoryProperties(physDevice, &memProperties);
				this->indices = GetIndices(physDevice, surface);
				return;
			}
		}

		// if we are here we failed to find a suitable physical device
		Assert(false, "Failed to create find a suitable physical device");
	}

	void Device::BuildLogicalDevice(VkQueue* presentQueue)
	{
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily, indices.transferFamily, indices.computeFamily };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};

		// Enables bindless (core in vulkan 1.2)
		
		VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexing{};
		descriptorIndexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		descriptorIndexing.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
		descriptorIndexing.runtimeDescriptorArray = VK_TRUE;
		descriptorIndexing.descriptorBindingVariableDescriptorCount = VK_TRUE;
		descriptorIndexing.descriptorBindingPartiallyBound = VK_TRUE;

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(uniqueQueueFamilies.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.physExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = extensions.physExtensions.data();
		deviceCreateInfo.pNext = &descriptorIndexing;

		if (!extensions.validationLayers.empty())
		{
			deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(extensions.validationLayers.size());
			deviceCreateInfo.ppEnabledLayerNames = extensions.validationLayers.data();
		}
		else deviceCreateInfo.enabledLayerCount = 0;

		VkPhysicalDeviceProperties props = {};

		vkGetPhysicalDeviceProperties(physDevice, &props);

		auto success = vkCreateDevice(physDevice, &deviceCreateInfo, nullptr, &device);
		Assert(success == VK_SUCCESS, "Failed to create logical device");

		volkLoadDevice(device);

		vkGetDeviceQueue(device, indices.graphicsFamily, 0, &queues.graphics);
		vkGetDeviceQueue(device, indices.presentFamily, 0, presentQueue);

		if (indices.transferFamily != -1) vkGetDeviceQueue(device, indices.transferFamily, 0, &queues.transfer);
		else LogInfo("Seperate transfer command queues not supported");

		LogInfo("Using Physical Device: {}", props.deviceName);
		LogInfo("	- Vender API version: {}", props.apiVersion);
		LogInfo("	- Driver version:  {}", props.driverVersion);
		LogInfo("");

		LogInfo("Queues: ");
		LogInfo("	- Graphics Family Index {}", indices.graphicsFamily);
		LogInfo("	- Present Family Index {}", indices.presentFamily);
		LogInfo("	- Compute Family Index {}", indices.computeFamily);
		LogInfo("	- Transfer Family Index {}", indices.transferFamily);
	}


	QueueFamilyIndices Device::GetIndices(VkPhysicalDevice physDevice, VkSurfaceKHR* surface)
	{
		QueueFamilyIndices indices;
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			// queue can copy/blit (transfer), but not graphics or compute.
			if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
			{
				indices.transferFamily = i;
				break; // No need to keep searching if we have found a queue that can only transfer
			}
			i++;
		}

		i = 0;

		for (const auto& queueFamily : queueFamilies)
		{
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, i, *surface, &presentSupport);

			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) indices.graphicsFamily = i;
			if (queueFamily.queueCount && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) indices.computeFamily = i;
			if (queueFamily.queueCount > 0 && presentSupport) indices.presentFamily = i;

			i++;
		}

		return indices;
	}

	bool Device::CheckDeviceExtensionSupport(VkPhysicalDevice physDevice)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(extensions.physExtensions.begin(), extensions.physExtensions.end());

		for (const auto& extension : availableExtensions) requiredExtensions.erase(extension.extensionName);

		return requiredExtensions.empty();
	}

	void Device::CheckSwapChainSupport(VkPhysicalDevice physDevice, VkSurfaceKHR* surface, VkSurfaceCapabilitiesKHR* capabilities, std::vector<VkSurfaceFormatKHR>& formats, std::vector<VkPresentModeKHR>& presentModes)
	{
		// load device SwapChain capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, *surface, capabilities);

		// load device SwapChain formats
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, *surface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, *surface, &formatCount, formats.data());
		}

		// load device SwapChain presentcount
		uint32_t presentCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, *surface, &presentCount, nullptr);

		if (presentCount != 0)
		{
			presentModes.resize(presentCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, *surface, &presentCount, presentModes.data());
		}
	}

	bool Device::IsDeviceSuitable(VkPhysicalDevice physDevice, VkSurfaceKHR* surface, VkSurfaceCapabilitiesKHR* capabilities, std::vector<VkSurfaceFormatKHR>& formats, std::vector<VkPresentModeKHR>& presentModes)
	{
		VkPhysicalDeviceProperties physDeviceProperties;
		VkPhysicalDeviceFeatures physDeviceFeatures;
		vkGetPhysicalDeviceProperties(physDevice, &physDeviceProperties);
		vkGetPhysicalDeviceFeatures(physDevice, &physDeviceFeatures);

		QueueFamilyIndices indices = GetIndices(physDevice, surface);

		bool extensionsSupported = CheckDeviceExtensionSupport(physDevice);
		bool swapChainSupport = false;
		if (extensionsSupported)
		{
			CheckSwapChainSupport(physDevice, surface, capabilities, formats, presentModes);
			swapChainSupport = !formats.empty() && !presentModes.empty();
		}

		return physDeviceFeatures.geometryShader && indices.isComplete() && extensionsSupported && swapChainSupport;
	}
}
