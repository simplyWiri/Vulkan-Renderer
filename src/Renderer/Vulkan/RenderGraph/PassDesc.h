#pragma once
#include <functional>
#include <optional>


#include "GraphContext.h"
#include "Tether.h"
#include "../Wrappers/Swapchain.h"

namespace Renderer
{
	struct PassDesc
	{
		public:
			PassDesc() { }

			PassDesc& SetName(const char* name)
			{
				this->taskName = name;
				return *this;
			}

			PassDesc& SetTarget(RenderpassKey key)
			{
				this->target = key;
				return *this;
			}

			PassDesc& SetExtent(VkExtent2D extent)
			{
				this->extent = extent;
				return *this;
			}

			PassDesc& SetInitialisationFunc(std::function<void(Tether&)> func)
			{
				this->initialisation = func;
				return *this;
			}

			PassDesc& SetRecordFunc(std::function<void(VkCommandBuffer, const FrameInfo&, GraphContext&)> func)
			{
				this->execute = func;
				return *this;
			}

			std::string taskName;
			std::optional<VkExtent2D> extent;
			std::optional<RenderpassKey> target;
			std::function<void(Tether&)> initialisation;
			std::function<void(VkCommandBuffer, const FrameInfo&, GraphContext&)> execute;
	};
}
