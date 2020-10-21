#pragma once
#include <string>
#include <vector>
#include <vulkan.h>
#include <glm/glm.hpp>

namespace Renderer
{
	enum class ImageSizeClass;
	
	class Resource
	{
	public:
		explicit Resource(uint32_t id) : id(id) { }

		uint32_t GetId() { return id; }
		void SetName(const std::string& string) { this->name = string; }
		void SetMemoryUsage(VkMemoryPropertyFlags flags) { this->memoryFlags = flags; }
		void CreatedBy(uint32_t createdByPass) { this->createdByPass = createdByPass; }

		void ReadBy(uint32_t pass) { readBy.emplace_back(pass); }
		void WrittenTo(uint32_t pass) { writtenBy.emplace_back(pass); }
		
	private:
		uint32_t id;
		uint32_t createdByPass;
		std::string name;
		VkMemoryPropertyFlags memoryFlags;

		std::vector<uint32_t> readBy;
		std::vector<uint32_t> writtenBy;
	};

	struct ImageInfo
	{
		VkImageUsageFlags usage = 0;
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		ImageSizeClass sizeClass = static_cast<ImageSizeClass>(0);
		glm::vec3 size = { -1, -1, -1};
		uint32_t samples = 1;
		uint32_t levels = 1;
		uint32_t layers = 1;
	};
	
	
	class ImageResource : public Resource
	{
		VkImageUsageFlags usage;
		VkFormat format;
		VkImageLayout layout;
		ImageSizeClass sizeClass;
		glm::vec3 size;
		uint32_t samples;
		uint32_t levels;
		uint32_t layers;
		
	public:
		ImageResource(uint32_t id) : Resource(id) { }

		void AssignInformation(const ImageInfo& info)
		{
			this->usage = info.usage;
			this->format = info.format;
			this->layout = info.layout;
			this->sizeClass = info.sizeClass;
			this->size = info.size;
			this->samples = info.samples;
			this->levels = info.levels;
			this->layers = info.layers;
		}
	};

	struct BufferInfo
	{
		VkBufferUsageFlags usage = 0;
		VkDeviceSize size = VkDeviceSize(0);
	};

	class BufferResource : public Resource
	{
		VkBufferUsageFlags usage;
		VkDeviceSize size;
		
	public:
		BufferResource(uint32_t id) : Resource(id) { }

		void AssignInformation(const BufferInfo& info)
		{
			this->usage = info.usage;
			this->size = info.size;
		}
	};
}