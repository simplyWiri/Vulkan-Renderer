#include "Allocation.h"
#include "Block.h"

namespace Renderer::Memory
{
	Allocation::Allocation(Block* parent, const VkDeviceSize& size, const VkDeviceSize& offset, bool used) : parent(parent), inUse(used), size(size), offset(offset) { }

	uint8_t* Allocation::Map() { return static_cast<uint8_t*>(parent->Map()) + offset; }
	void Allocation::Unmap() { parent->Unmap(); }
}
