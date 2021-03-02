#pragma once
#include <functional>

#include "volk/volk.h"

#include "MemoryResource.h"

namespace Renderer::Memory
{
	class Buffer : public MemoryResource<VkBuffer>
	{
		friend struct std::hash<Buffer>;
		friend class Allocator;
	private:
		std::function<void(Buffer*)> cleanup;
		VkBufferUsageFlags usage;
		VkMemoryPropertyFlags flags;
		VkDeviceSize size;

	public:
		operator VkBuffer() { return resourceHandle; }
		VkBufferUsageFlags GetUsageFlags() const { return usage; }
		VkMemoryPropertyFlags GetMemoryFlags() const { return flags; }
		VkDeviceSize GetSize() const { return size; }

	public:
		Buffer(const Allocation& alloc, VkBuffer buff, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, const VkDeviceSize& size, const std::function<void(Buffer*)>& cleanup);
		~Buffer();

		uint8_t* Map();
		void Unmap();

		void Load(const void* data, const size_t& size, const size_t& offset = 0);

		bool operator==(const Buffer& other) const { return std::tie(resourceHandle, allocation, usage, flags) == std::tie(other.resourceHandle, other.allocation, other.usage, other.flags); }

		static Buffer* Resize(Allocator* allocator, Buffer* buffer, const VkDeviceSize& size);
	};
}

namespace std
{
	template <>
	struct hash<Renderer::Memory::Buffer>
	{
		size_t operator()(const Renderer::Memory::Buffer& s) const noexcept
		{
			std::size_t seed = 0x57C06A1D;
			seed ^= (seed << 6) + (seed >> 2) + 0x488410ED + reinterpret_cast<std::uint64_t>(s.resourceHandle);
			seed ^= (seed << 6) + (seed >> 2) + 0x2FC76560 + static_cast<std::size_t>(s.usage);
			seed ^= (seed << 6) + (seed >> 2) + 0x4E7B13D1 + static_cast<std::size_t>(s.flags);
			return seed;
		}
	};
}
