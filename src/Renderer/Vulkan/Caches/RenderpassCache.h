#pragma once
#include "vulkan.h"
#include "../Wrappers/Renderpass.h"
#include "Cache.h"

namespace Renderer
{
	class RenderpassCache : public Cache<Renderpass, RenderpassKey>
	{
	public:
		void buildCache(VkDevice* device) { this->device = device; }

		Renderpass* get(const RenderpassKey& key) override
		{
			auto& renderPass = cache[key];
			if (!renderPass)
			{
				renderPass = new Renderpass(device, key);
				registerInput(key);
			}
			return renderPass;
		}

		bool add(const RenderpassKey& key) override
		{
			if (cache.find(key) != cache.end()) return false;

			cache.emplace(key, new Renderpass(device, key));
			registerInput(key);

			return true;
		}

		bool add(const RenderpassKey& key, uint16_t& local) override
		{
			if (cache.find(key) != cache.end()) return false;

			cache.emplace(key, new Renderpass(device, key));
			local = registerInput(key);

			return true;
		}

		void clearEntry(Renderpass* renderpass) override
		{
			vkDestroyRenderPass(*device, renderpass->getHandle(), nullptr);
		}

	private:
		VkDevice* device;
	};
}
