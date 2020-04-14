#pragma once
#include "glm/glm.hpp"
#include "vulkan.h"
#include <vector>

namespace Renderer {
	// How is the image attachment sized relative to swapchain?
	enum SizeClass { Absolute, SwapchainRelative, Input };

	// Image Attachment
	struct ImageAttachmentInfo
	{
		SizeClass sizeClass = SizeClass::SwapchainRelative;
		glm::vec3 size = { 1.0f, 1.0f, 0.0f };
		VkFormat format = VK_FORMAT_UNDEFINED;
		uint32_t samples = 1, levels = 1, layers = 1;
		bool persistent = true;
		VkImageUsageFlags usage = 0;
	};

	// Buffer Attachment
	struct BufferAttachmentInfo
	{
		VkDeviceSize size = 0;
		VkBufferUsageFlags usage = 0;
		bool persistent = true;
	};

	class Rendergraph
	{
	};
}
