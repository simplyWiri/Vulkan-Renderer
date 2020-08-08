#pragma once
#include "Cache.h"
#include "../Wrappers/DescriptorSet.h"
#include "../Wrappers/Device.h"
#include "vulkan.h"

namespace Renderer
{
	class DescriptorSetCache : public Cache<DescriptorSetBundle, DescriptorSetKey>
	{
		private:
			VkDevice* device;
			VmaAllocator* allocator;
			uint32_t framesInFlight;
		public:
			void buildCache(VkDevice* device, VmaAllocator* allocator, uint32_t framesInFlight)
			{
				this->device = device;
				this->allocator = allocator;
				this->framesInFlight = framesInFlight;
			}

			void writeBuffer(DescriptorSetKey& key, const std::string& resName)
			{
				auto descBundle = get(key);

				descBundle->writeBuffer(resName);
			}

			void writeBuffer(DescriptorSetKey& key, const std::string& resName, Buffer* buffer)
			{
				auto descBundle = get(key);

				descBundle->writeBuffer(resName, buffer);
			}

			void writeSampler(DescriptorSetKey& key, const std::string& resName) { }

			void writeSampler(DescriptorSetKey& key, const std::string& resName, Image* image, Sampler* sampler, VkImageLayout layout)
			{
				auto descBundle = get(key);

				descBundle->writeSampler(resName, image, sampler, layout);
			}

			template <typename T>
			T* getResource(const DescriptorSetKey& key, std::string resName, uint32_t offset)
			{
				auto descSet = get(key);

				return static_cast<T*>(descSet->getResource(resName, offset));
			}

			void setResource(const DescriptorSetKey& key, std::string resName, uint32_t offset, void* data, size_t size)
			{
				auto descSet = get(key);

				descSet->setResource(resName, data, size, offset);
			}

			void bindDescriptorSet(VkCommandBuffer buffer, VkPipelineBindPoint bindPoint, const DescriptorSetKey& key, uint32_t offset)
			{
				auto descSet = get(key);

				uint32_t dynOffset = 0;
				if (key.program->getResources().size() == 1) vkCmdBindDescriptorSets(buffer, bindPoint, key.program->getPipelineLayout(), 0, 1, descSet->get(offset), 1, &dynOffset);
				else vkCmdBindDescriptorSets(buffer, bindPoint, key.program->getPipelineLayout(), 0, 1, descSet->get(offset), 0, nullptr);
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
				set->clear();
			}
	};
}
