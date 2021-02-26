#pragma once
#include <glm/glm.hpp>
#include <vulkan.h>
#include <string>
#include <vector>


namespace Renderer
{
	namespace Memory
	{
		class Buffer;
		class Allocator;
		class Image;
	}
}

namespace Renderer::RenderGraph
{
	struct BufferInfo
	{
		VkBufferUsageFlags usage = 0;
		VkDeviceSize size = static_cast<VkDeviceSize>(0);
	};

	struct ImageInfo
	{
		enum class SizeType : char { Swapchain = 1 << 0, Fixed = 1 << 1, Relative = 1 << 2 } sizeType = SizeType::Swapchain;

		VkImageUsageFlags usage = 0;
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		glm::vec3 size = { -1, -1, -1 };
		uint32_t samples = 1;
		uint32_t levels = 1;
		uint32_t layers = 1;

		ImageInfo& SetSize(glm::vec3 size);
		ImageInfo& SetFormat(VkFormat format);
		ImageInfo& SetLayout(VkImageLayout layout);
		ImageInfo& SetUsage(VkImageUsageFlags usage);
		ImageInfo& SetSizeType(SizeType sizeType);
	};


	struct Usage
	{
		uint32_t passId;

		VkPipelineStageFlags flags;
		VkAccessFlags access;
		uint32_t queueFamilyIndices;

		// For images, when 'using' we can also want to change the layout
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	};

	struct Resource
	{
		// What defines our resource
		std::string name;

		enum class Type { Buffer, Image } type;

		std::vector<Usage> reads;
		std::vector<Usage> writes;

		explicit Resource(const std::string& name) : name(name) { }

		void ReadBy(const Usage& usage) { reads.emplace_back(usage); }
		void WrittenBy(const Usage& usage) { writes.emplace_back(usage); }

		bool IsWritten() const { return !writes.empty(); }
		bool IsRead() const { return !reads.empty(); }
	};

	struct BufferResource : Resource
	{
		std::vector<Memory::Buffer*> buffers;
		BufferInfo info;

		BufferResource(const std::string& name) : Resource(name) { type = Type::Buffer; }
		~BufferResource();
		void SetInfo(BufferInfo info) { this->info = info; }

		void Build(Memory::Allocator* allocator, uint32_t framesInFlight);
	};

	struct ImageResource : Resource
	{
		std::vector<Memory::Image*> images;
		ImageInfo info;

		explicit ImageResource(const std::string& name) : Resource(name) { type = Type::Image; }
		~ImageResource();

		void SetInfo(ImageInfo info) { this->info = info; }

		void Build(Memory::Allocator* allocator, VkExtent2D swapchainExtent, uint32_t framesInFlight);
	};
}
