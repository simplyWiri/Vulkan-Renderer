#include "Allocation.h"
#include "Block.h"

namespace Renderer::Memory
{

Allocation::Allocation(Block* parent, const VkDeviceSize& size, const VkDeviceSize& offset, const uint32_t& id)
	: parent(parent), size(size), offset(offset), id(id)
{
	
}
	
uint8_t* Allocation::Map()
{
	return static_cast<uint8_t*>(parent->Map()) + offset;
}
void Allocation::Unmap()
{
	parent->Unmap();
}

}