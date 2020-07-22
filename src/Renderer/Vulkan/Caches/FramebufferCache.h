#pragma once
#include "vulkan.h"
#include "../Wrappers/Framebuffer.h"
#include "Cache.h"

namespace Renderer
{
	class FramebufferCache : public Cache<Framebuffer, FramebufferKey>
	{
	public:
		void buildCache(VkDevice* device) { this->device = device; }

		Framebuffer* get(const FramebufferKey& key) override
		{
			auto& renderPass = cache[key];
			if (!renderPass)
			{
				renderPass = new Framebuffer(device, key);
				registerInput(key);
			}
			return renderPass;
		}

		bool add(const FramebufferKey& key) override
		{
			if (cache.find(key) != cache.end()) return false;

			cache.emplace(key, new Framebuffer(device, key));
			registerInput(key);

			return true;
		}

		bool add(const FramebufferKey& key, uint16_t& local) override
		{
			if (cache.find(key) != cache.end()) return false;

			cache.emplace(key, new Framebuffer(device, key));
			local = registerInput(key);

			return true;
		}

	private:

		void clearEntry(Framebuffer* framebuffers) override
		{
			for (auto& framebuffer : framebuffers->getHandle())
			{
				vkDestroyFramebuffer(*device, framebuffer, nullptr);

			}
		}

		VkDevice* device;
	};
}
