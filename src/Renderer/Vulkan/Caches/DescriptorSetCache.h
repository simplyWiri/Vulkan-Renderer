#pragma once
#include "Cache.h"
#include "../Wrappers/DescriptorSet.h"
#include "../Wrappers/Device.h"

namespace Renderer
{
	class DescriptorSetCache : public Cache<DescriptorSetBundle, DescriptorSetKey>
	{
	private:
		VkDevice* device;
		VmaAllocator* allocator;
		uint32_t framesInFlight;
	public:
		void buildCache(VkDevice* device, VmaAllocator* allocator, uint32_t framesInFlight) { this->device = device; this->allocator = allocator; this->framesInFlight = framesInFlight; }


		template<typename T>
		T* getResource(const DescriptorSetKey& key, std::string resName, uint32_t offset)
		{
			auto descSet = get(key);

			return (T*)descSet->getResource(resName, offset);
		}
		void setResource(const DescriptorSetKey& key, std::string resName, uint32_t offset, void* data, size_t size)
		{
			auto descSet = get(key);

			descSet->setResource(resName, data, size, offset);
		}

		void bindDescriptorSet(VkCommandBuffer buffer, VkPipelineBindPoint bindPoint, VkPipelineLayout pLayout, const DescriptorSetKey& key, uint32_t offset)
		{
			auto descSet = get(key);

			uint32_t dynOffset = 0;
			vkCmdBindDescriptorSets(buffer, bindPoint, pLayout, 0, 1, descSet->get(offset), 1, &dynOffset);
		}

		DescriptorSetBundle* get(const DescriptorSetKey& key) override
		{
			auto& renderPass = cache[key];
			if (!renderPass)
			{
				renderPass = new DescriptorSetBundle(device, allocator, key, framesInFlight);
				registerInput(key);
			}
			return renderPass;
		}

		bool add(const DescriptorSetKey& key) override
		{
			if (cache.find(key) != cache.end()) return false;

			cache.emplace(key, new DescriptorSetBundle(device, allocator, key, framesInFlight));
			registerInput(key);

			return true;
		}

		bool add(const DescriptorSetKey& key, uint16_t& local) override
		{
			if (cache.find(key) != cache.end()) return false;

			cache.emplace(key, new DescriptorSetBundle(device, allocator, key, framesInFlight));
			local = registerInput(key);

			return true;
		}

	private:

		void clearEntry(DescriptorSetBundle* set) override
		{
			vkDestroyDescriptorPool(*device, set->getPool(), nullptr);
		}

	};

}