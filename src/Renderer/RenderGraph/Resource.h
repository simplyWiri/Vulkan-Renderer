#pragma once
#include <glm/glm.hpp>
#include <vulkan.h>
#include <string>
#include <vector>


namespace Renderer
{
	enum class ImageSize : char;
	
	namespace Memory
	{
		class Buffer;
		class Allocator;
		class Image;
	}

	struct BufferInfo
	{
		VkBufferUsageFlags usage = 0;
		VkDeviceSize size = static_cast<VkDeviceSize>(0);
	};
	 
	struct ImageInfo 
	{
		ImageSize sizeType = static_cast<ImageSize>(0);
		VkImageUsageFlags usage = 0;
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		glm::vec3 size = { -1, -1, -1};
		uint32_t samples = 1;
		uint32_t levels = 1;
		uint32_t layers = 1;

		ImageInfo& SetSize (glm::vec3 size);
		ImageInfo& SetFormat (VkFormat format);
		ImageInfo& SetLayout (VkImageLayout layout);
		ImageInfo& SetUsage (VkImageUsageFlags usage);
		ImageInfo& SetSizeType (ImageSize sizeType);
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

		std::vector<Usage> reads;
		std::vector<Usage> writes;

		Resource(const std::string& name) : name(name) { }
		
		void ReadBy(const Usage& usage) { reads.emplace_back(usage); }
		void WrittenBy(const Usage& usage) { writes.emplace_back(usage); }
	};

	struct BufferResource : Resource
	{
		std::vector<Memory::Buffer*> buffers; 
		BufferInfo info;

		BufferResource(const std::string& name) : Resource(name) {}

		void SetInfo(BufferInfo info) { this->info = info; }

		void Build(Memory::Allocator* allocator, VkExtent2D swapchainExtent, uint32_t framesInFlight);
	};

	struct ImageResource : Resource
	{
		std::vector<Memory::Image*> images;
		ImageInfo info;

		ImageResource(const std::string& name) : Resource(name) {}

		void SetInfo(ImageInfo info) { this->info = info; }

		void Build(Memory::Allocator* allocator, VkExtent2D swapchainExtent, uint32_t framesInFlight);
	};
}
