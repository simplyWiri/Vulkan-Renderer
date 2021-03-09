#pragma once
#include <string>
#include <vector>
#include <optional>

#include "glm/glm/vec3.hpp"
#include "volk/volk.h"


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
	enum class QueueType : char;
	
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
		glm::vec3 size{ -1 };
	};

	struct Access
	{
		Access(std::string passAlias, VkPipelineStageFlags stageFlags, VkAccessFlags accessType, VkImageLayout expectedLayout = VK_IMAGE_LAYOUT_UNDEFINED, VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_MAX_ENUM)
		: passAlias(passAlias), stageFlags(stageFlags), accessType(accessType), expectedLayout(expectedLayout), loadOp(loadOp) { }
		
		std::string passAlias;

		VkPipelineStageFlags stageFlags;

		VkAccessFlags accessType;
		
		bool IsWrite() const { return accessType & VK_ACCESS_SHADER_WRITE_BIT;}
		bool IsRead() const { return accessType & VK_ACCESS_SHADER_READ_BIT; }

		// Image Only

		// The layout this image is expected to be in when it will be accessed
		VkImageLayout expectedLayout;

		// The loadop used on the image if its being read as a color attachment
		VkAttachmentLoadOp loadOp;

		bool RequiresPriorWrite() const { return loadOp != VK_ATTACHMENT_LOAD_OP_CLEAR; }

		bool operator==(const Access& other) const
		{
			return std::tie(passAlias, stageFlags, accessType, expectedLayout, loadOp) == 
				   std::tie(other.passAlias, other.stageFlags, other.accessType, other.expectedLayout, other.loadOp);
		}
	};
	
	struct ResourceDescription
	{
		explicit ResourceDescription(const std::string& name) : name(name) { }

		std::string name;
		enum class Type { Buffer, Image } type;

		std::vector<Access> accesses;


		std::optional<Access> ReadByPass(const std::string& passAlias) const
		{
			for(const auto& access : accesses)
				if(access.IsRead() && access.passAlias == passAlias) return access;

			return std::nullopt;
		}

		std::optional<Access> WrittenByPass(const std::string& passAlias) const
		{
			for(const auto& access : accesses)
				if(access.IsWrite() && access.passAlias == passAlias) return access;

			return std::nullopt;
		}

		std::vector<Access> GetWrites() const
		{
			std::vector<Access> writes;

			for(const auto& access : accesses)
				if(access.IsWrite()) writes.emplace_back(access);

			return writes;
		}

		std::vector<Access> GetReads() const
		{
			std::vector<Access> reads;

			for(const auto& access : accesses)
				if(access.IsRead()) reads.emplace_back(access);

			return reads;
		}

		std::vector<Access> SortedAccesses(const std::vector<std::string>& sortedPasses) const
		{
			std::vector<Access> sortedAccess;

			for(const auto& pass : sortedPasses)
				for(const auto& access : accesses)
					if(access.passAlias == pass) sortedAccess.emplace_back(access);

			return sortedAccess;
		}

		// We assume that both passes write to this image, and one of them loads the image, the other clears
		// The pass returned is the pass which needs to execute first.
		std::string OrderPassesByWrites(const std::string& firstAlias, const std::string& secondAlias) const
		{
			for(const auto& access : accesses)
			{				
				if(access.IsRead()) continue;

				if(access.RequiresPriorWrite())
				{
					if(access.passAlias == firstAlias) return secondAlias;
					if(access.passAlias == secondAlias) return firstAlias;
				}
			}
			
			return "";
		}
		
		void AccessedBy(const Access& access)
		{
			accesses.emplace_back(access);
		}
	};

	struct BufferResource : ResourceDescription
	{
		BufferInfo info;

		explicit BufferResource(const std::string& name) : ResourceDescription(name) { type = Type::Buffer; }
		void SetInfo(BufferInfo info) { this->info = info; }
	};

	struct ImageResource : ResourceDescription
	{
		ImageInfo info;

		explicit ImageResource(const std::string& name) : ResourceDescription(name) { type = Type::Image; }
		void SetInfo(ImageInfo info) { this->info = info; }
	};

}
