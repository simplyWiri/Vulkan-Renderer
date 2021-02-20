#include "Buffer.h"
#include "Allocator.h"

namespace Renderer::Memory
{
	Buffer::Buffer(const Allocation& alloc, VkBuffer buff, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags flags, const VkDeviceSize& size, const std::function<void(Buffer*)>& cleanup) : MemoryResource(alloc, buff),
		cleanup(cleanup), usage(usage), flags(flags), size(size) { }

	Buffer::~Buffer() { cleanup(this); }

	uint8_t* Buffer::Map() { return allocation.Map(); }
	void Buffer::Unmap() { allocation.Unmap(); }

	void Buffer::Load(const void* data, const size_t& size, const size_t& offset)
	{
		auto dest = Map() + offset;
		memcpy(dest, data, size);
		Unmap();
	}

	Buffer* Buffer::Resize(Allocator* allocator, Buffer* buffer, const VkDeviceSize& size)
	{
		if (buffer->GetSize() < size)
		{
			auto usage = buffer->GetUsageFlags();
			auto flags = buffer->GetMemoryFlags();
			delete buffer;
			buffer = allocator->AllocateBuffer(size, usage, flags);
		}

		return buffer;
	}
}
