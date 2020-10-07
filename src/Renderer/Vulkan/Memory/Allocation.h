#pragma once
#include <cstddef>
#include <cstdint>
#include <tuple>

#include "vulkan.h"

namespace Renderer::Memory
{
	class Block;

	class Allocation
	{
		friend class FreeList;
		friend class Block;
		friend class Allocator;
	private:
		Block* parent;
		std::pair<int, int> freeListsIndexes;
		
		uint32_t id;
		VkDeviceSize size;
		VkDeviceSize offset;

	public:
		Block* GetParent() const { return parent; }
		VkDeviceSize GetSize() const { return size; }
		VkDeviceSize GetOffset() const { return offset; }
		
	public:
		Allocation(Block* parent, const VkDeviceSize& size, const VkDeviceSize& offset, const uint32_t& id);
		~Allocation() = default;

		uint8_t* Map();
		void Unmap();

		bool operator==(const Allocation& other) const
		{
			return std::tie(parent, size, offset) == std::tie(other.parent, other.size, other.offset);
		}

		bool operator<(const Allocation& other) const
		{
			if(size != other.size) return size < other.size;
			return offset < other.offset;
		}
	};
}


namespace std
{
	template<>
	struct hash<Renderer::Memory::Allocation>
	{
		size_t operator()(const Renderer::Memory::Allocation& s) const noexcept
		{
			size_t seed = 0x4A18DFA2;
			seed ^= (seed << 6) + (seed >> 2) + 0x38FE3190 + reinterpret_cast<std::size_t>(s.GetParent());
			seed ^= (seed << 6) + (seed >> 2) + 0x2BB790AF + static_cast<std::size_t>(s.GetSize());
			seed ^= (seed << 6) + (seed >> 2) + 0x5CEDA39A + static_cast<std::size_t>(s.GetOffset());
			return seed;
		}
	};
}
