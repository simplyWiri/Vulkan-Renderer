#pragma once
#include "vulkan.h"
#include "vk_mem_alloc.h"


namespace Renderer
{
	class Allocator
	{
		enum ResourceType
		{
			Buffer, Image
		};
		
	public:
		

		
	private:
		VmaAllocator allocator;

		size_t totalUtilised;
		size_t totalAllocation;

	public:


	private:
		void allocateChunk(const size_t& size, const ResourceType& type)
		{
			
		}
		
	};
}