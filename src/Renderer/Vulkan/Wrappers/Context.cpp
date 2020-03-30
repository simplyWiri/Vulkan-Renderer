#include "Context.h"
#include "glfw3.h"
#include "Window.h"
#include <set>
#include <string>
#include <iostream>

// Instance related functions
const char* strMessageSeverity(VkDebugUtilsMessageSeverityFlagBitsEXT s) {
	switch (s) {
	case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		return "VERBOSE";
	case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		return "ERROR";
	case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		return "WARNING";
	case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		return "INFO";
	default:
		return "UNKNOWN";
	}
}
const char* strMessageType(VkDebugUtilsMessageTypeFlagsEXT s) {
	if (s == 7) return "General | Validation | Performance";
	if (s == 6) return "Validation | Performance";
	if (s == 5) return "General | Performance";
	if (s == 4 /*VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT*/) return "Performance";
	if (s == 3) return "General | Validation";
	if (s == 2 /*VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT*/) return "Validation";
	if (s == 1 /*VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT*/) return "General";
	return "Unknown";
}
VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
	auto ms = strMessageSeverity(messageSeverity);
	auto mt = strMessageType(messageType);
	printf("[%s: %s]\n%s\n", ms, mt, pCallbackData->pMessage);

	return VK_FALSE;
}
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

// Physical Device related functions
QueueFamilyIndices getIndices(VkPhysicalDevice physDevice, Window* window)
{
	QueueFamilyIndices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		// queue can copy/blit (transfer), but not graphics or compute.
		if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
			((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) &&
			((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
		{
			indices.transferFamily = i;
		}
		i++;
	}

	i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = i;
		if (queueFamily.queueCount && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
			indices.computeFamily = i;

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, i, window->getSurface(), &presentSupport);
		if (queueFamily.queueCount > 0 && presentSupport)
			indices.presentFamily = i;

		i++;
	}

	// in case there is no family which can only transfer
	if (indices.transferFamily == -1)
		indices.transferFamily = 0;

	return indices;
}
bool checkDeviceExtensionSupport(VkPhysicalDevice physDevice, Context* context)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(context->settings.physExtensions.begin(), context->settings.physExtensions.end());

	for (const auto& extension : availableExtensions)
		requiredExtensions.erase(extension.extensionName);

	return requiredExtensions.empty();
}
SwapChainSupportDetails checkSwapChainSupport(VkPhysicalDevice physDevice, Window* window)
{
	SwapChainSupportDetails swapDetails;
	// load device SwapChain capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, window->getSurface(), &swapDetails.capabilities);

	// load device SwapChain formats
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, window->getSurface(), &formatCount, nullptr);
	if (formatCount != 0) {
		swapDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, window->getSurface(), &formatCount, swapDetails.formats.data());
	}

	// load device SwapChain presentcount
	uint32_t presentCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, window->getSurface(), &presentCount, nullptr);

	if (presentCount != 0) {
		swapDetails.presentModes.resize(presentCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, window->getSurface(), &presentCount, swapDetails.presentModes.data());
	}
	return swapDetails;
}
bool isDeviceSuitable(VkPhysicalDevice physDevice, Context* context, Window* window)
{
	VkPhysicalDeviceProperties physDeviceProperties;
	VkPhysicalDeviceFeatures physDeviceFeatures;
	vkGetPhysicalDeviceProperties(physDevice, &physDeviceProperties);
	vkGetPhysicalDeviceFeatures(physDevice, &physDeviceFeatures);

	QueueFamilyIndices indices = getIndices(physDevice, window);

	bool extensionsSupported = checkDeviceExtensionSupport(physDevice, context);
	bool swapChainSupport = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapDetails = checkSwapChainSupport(physDevice, window);
		swapChainSupport = !swapDetails.formats.empty() && !swapDetails.presentModes.empty();
	}
	VkPhysicalDeviceType physType;
	switch (context->settings.pref)
	{
	case PhysicalDevicePreference::CPU:
		physType = VK_PHYSICAL_DEVICE_TYPE_CPU;
		break;
	case PhysicalDevicePreference::Discrete:
		physType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
		break;
	case PhysicalDevicePreference::Integrated:
		physType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
		break;
	case PhysicalDevicePreference::None:
		return physDeviceFeatures.geometryShader
			&& indices.isComplete()
			&& extensionsSupported
			&& swapChainSupport;
	default:
		// Currently not supporting Virtual / Multi GPU's
		return false;
		break;
	}

	return physDeviceProperties.deviceType == physType
		&& physDeviceFeatures.geometryShader
		&& indices.isComplete()
		&& extensionsSupported
		&& swapChainSupport;
}

namespace Wrappers {
	bool buildInstance(Context* context)
	{
		{
			// Get our required glfw extensions
			uint32_t glfwExtensionCount = 0;
			const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
			std::vector<const char*> glfwExts(glfwExtensions, glfwExtensions + glfwExtensionCount);
			for (auto extension : glfwExts)
				context->settings.instanceExtensions.emplace_back(extension);
		}

		VkApplicationInfo info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
		info.pApplicationName = "Application Name";
		info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		info.pEngineName = "Engine Name";
		info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		info.apiVersion = VK_MAKE_VERSION(context->settings.apiMajor, context->settings.apiMinor, 0);

		VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		createInfo.pApplicationInfo = &info;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(context->settings.instanceExtensions.size());
		createInfo.ppEnabledExtensionNames = context->settings.instanceExtensions.data();

		// Enable validation layers if req'd
		VkDebugUtilsMessengerCreateInfoEXT messengerInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
		if (context->settings.enableValidationLayers) {
			messengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			messengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			messengerInfo.pfnUserCallback = debugCallback;

			createInfo.enabledLayerCount = static_cast<uint32_t>(context->settings.validationLayers.size());
			createInfo.ppEnabledLayerNames = context->settings.validationLayers.data();
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&messengerInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &context->instance) != VK_SUCCESS)
			return false;

		if (CreateDebugUtilsMessengerEXT(context->instance, &messengerInfo, nullptr, &context->debugMessenger) != VK_SUCCESS)
			return false;

		return true;
	}

	bool buildDevice(Context* context)
	{
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<int> uniqueQueueFamilies = { context->gpu.indices.graphicsFamily, context->gpu.indices.presentFamily, context->gpu.indices.transferFamily, context->gpu.indices.computeFamily };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(uniqueQueueFamilies.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(context->settings.physExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = context->settings.physExtensions.data();

		if (!context->settings.validationLayers.empty()) {
			deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(context->settings.validationLayers.size());
			deviceCreateInfo.ppEnabledLayerNames = context->settings.validationLayers.data();
		}
		else
			deviceCreateInfo.enabledLayerCount = 0;

		VkPhysicalDeviceProperties props = {};

		vkGetPhysicalDeviceProperties(context->gpu.physDevice, &props);
		std::cout << "Using Physical Device: " << props.deviceName << std::endl;
		std::cout << "	- Vender API version: " << props.apiVersion << std::endl;
		std::cout << "	- Driver version: " << props.driverVersion << "\n\n\n";


		if (vkCreateDevice(context->gpu.physDevice, &deviceCreateInfo, nullptr, &context->device.device) != VK_SUCCESS)
			throw std::runtime_error("Failed to initialise logical device");

		vkGetDeviceQueue(context->device.device, context->gpu.indices.graphicsFamily, 0, &context->device.graphicsQueue);
		vkGetDeviceQueue(context->device.device, context->gpu.indices.presentFamily, 0, &context->device.presentQueue);

		if (context->gpu.indices.transferFamily != -1)
			vkGetDeviceQueue(context->device.device, context->gpu.indices.transferFamily, 0, &context->device.transferQueue);
		else {
			std::cout << "Transfer command queues not supported" << std::endl;
			return false;
		}
		if (context->gpu.indices.computeFamily != -1)
			vkGetDeviceQueue(context->device.device, context->gpu.indices.computeFamily, 0, &context->device.computeQueue);
		else {
			std::cout << "Compute command queues not supported" << std::endl;
			return false;
		}
		return false;
	}

	bool pickPhysicalDevice(Context* context, Window* window)
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(context->instance, &deviceCount, nullptr);
		if (deviceCount == 0) // No GPU's provide support for vulkan!
			return false;

		std::vector<VkPhysicalDevice> physDevices(deviceCount);
		vkEnumeratePhysicalDevices(context->instance, &deviceCount, physDevices.data());

		for (const auto& physDevice : physDevices) {
			if (isDeviceSuitable(physDevice, context, window)) {
				context->gpu.physDevice = physDevice;
				context->gpu.indices = getIndices(physDevice, window);
				context->swapchain.details = checkSwapChainSupport(physDevice, window);
				return true;
			}
		}
		// if we are here we failed to find a suitable physical device

		return false;
	}
}