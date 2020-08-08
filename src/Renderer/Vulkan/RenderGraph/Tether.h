#pragma once
#include <vector>

#include "../../Resources/ShaderManager.h"
#include "../Caches/DescriptorSetCache.h"

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
