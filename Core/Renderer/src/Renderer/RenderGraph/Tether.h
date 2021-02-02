#pragma once

#include "../VulkanObjects/DescriptorSet.h"

namespace Renderer
{
	class Rendergraph;

	struct Tether
	{
	private:

		DescriptorSetCache* descriptorCache;

	public:

		DescriptorSetCache* GetDescriptorCache() { return descriptorCache; }

		friend class Rendergraph;
	};
}
