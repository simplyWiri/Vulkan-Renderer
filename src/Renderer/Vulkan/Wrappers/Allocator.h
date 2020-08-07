#pragma once
#include "vulkan.h"
#include "vk_mem_alloc.h"


namespace Renderer
{
	struct FrameInfo;

	class Allocator
	{
		enum ResourceType
		{
			Buffer,
			Image
		};

		public:


		private:
			VmaAllocator allocator;

			size_t totalUtilised;
			size_t totalAllocation;

		public:

			void beginFrame(const FrameInfo& frameInfo) { }

			void endFrame() { }

		private:
			void allocateChunk(const size_t& size, const ResourceType& type) { }
	};
}
