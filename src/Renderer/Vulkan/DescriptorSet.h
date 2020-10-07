#pragma once
#include <vector>
#include <vulkan.h>

#include "../../Utils/Logging.h"
#include "../Resources/Buffer.h"
#include "../Resources/Shader.h"
#include <glm/gtc/matrix_transform.hpp>

#include "../Resources/Image.h"
#include "../Resources/Sampler.h"
#include "../Cache.h"
#include "Memory/Allocator.h"


namespace Renderer
{
	struct DescriptorSetKey
	{
		ShaderProgram* program;

		bool operator ==(const DescriptorSetKey& other) const { return program == other.program; }
	};

	class DescriptorSetBundle
	{
	public:
		DescriptorSetBundle(VkDevice* device, Memory::Allocator* allocator, DescriptorSetKey key, uint32_t framesInFlight) : framesInFlight(framesInFlight), resources(key.program->getResources()), device(device), allocator(allocator)
		{
			std::vector<VkDescriptorPoolSize> poolSizes;

			// initialise our pools
			for (const auto& resource : key.program->getResources())
			{
				if (resource.type == VK_DESCRIPTOR_TYPE_MAX_ENUM) continue; // this is a push constant

				VkDescriptorPoolSize poolSize = {};
				poolSize.type = resource.type;
				poolSize.descriptorCount = framesInFlight;

				poolSizes.emplace_back(poolSize);
			}

			VkDescriptorPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
			poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			poolCreateInfo.pPoolSizes = poolSizes.data();
			poolCreateInfo.maxSets = framesInFlight;
			sets.resize(framesInFlight);

			auto success = vkCreateDescriptorPool(*device, &poolCreateInfo, nullptr, &pool);
			Assert(success == VK_SUCCESS, "Failed to create descriptor pool");

			std::vector<VkDescriptorSetLayout> layouts(framesInFlight, key.program->getDescriptorLayout());

			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = pool;
			allocInfo.descriptorSetCount = framesInFlight;
			allocInfo.pSetLayouts = layouts.data();

			sets.resize(framesInFlight);
			success = vkAllocateDescriptorSets(*device, &allocInfo, sets.data());
			Assert(success == VK_SUCCESS, "Failed to allocator descriptor sets");
		}

		void WriteBuffer(const std::string& resName)
		{
			const auto res = GetShaderResource(resName);
			uint32_t size = res.size;
			
			size = std::max(256U, size); // 256U is the min alignment req

			switch (res.type)
			{
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: WriteBuffer(resName, allocator->AllocateBuffer(size * framesInFlight, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )); break;
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: WriteBuffer(resName, allocator->AllocateBuffer(size * framesInFlight, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)); break;
			}
		}

		void WriteBuffer(const std::string& resName, Memory::Buffer* buffer)
		{
			buffers.emplace(resName, buffer);

			const auto res = GetShaderResource(resName);

			if (buffer->GetSize() < res.size * framesInFlight)
			{
				auto usage = buffer->GetUsageFlags();
				auto flags = buffer->GetMemoryFlags();
				delete buffer;
				buffer = allocator->AllocateBuffer(res.size * framesInFlight, usage, flags);
			}

			for (uint32_t i = 0; i < framesInFlight; i++)
			{
				std::vector<VkWriteDescriptorSet> writeSets;

				VkDescriptorBufferInfo descBufferInfo = {};
				descBufferInfo.buffer = buffer->GetResourceHandle();
				descBufferInfo.offset = i * std::max(256U, res.size);
				descBufferInfo.range = std::max(256U, res.size);

				VkWriteDescriptorSet writeDescSet = {};
				writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescSet.dstSet = sets[i];
				writeDescSet.dstBinding = res.binding;
				writeDescSet.dstArrayElement = 0;
				writeDescSet.descriptorType = res.type;
				writeDescSet.descriptorCount = res.descriptorCount;
				writeDescSet.pBufferInfo = &descBufferInfo;

				writeSets.push_back(writeDescSet);

				vkUpdateDescriptorSets(*device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
			}
		}

		void WriteSampler(const std::string& resName, Image* image, Sampler* sampler, VkImageLayout layout)
		{
			const auto res = GetShaderResource(resName);

			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageView = image->GetView();
			images.emplace(resName, image);
			imageInfo.sampler = sampler->getSampler();
			samplers.emplace(resName, sampler);
			imageInfo.imageLayout = layout;

			for (uint32_t i = 0; i < framesInFlight; i++)
			{
				std::vector<VkWriteDescriptorSet> writeSets;

				VkWriteDescriptorSet writeDescSet = {};
				writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescSet.dstSet = sets[i];
				writeDescSet.dstBinding = res.binding;
				writeDescSet.dstArrayElement = 0;
				writeDescSet.descriptorType = res.type;
				writeDescSet.descriptorCount = res.descriptorCount;
				writeDescSet.pImageInfo = &imageInfo;

				writeSets.push_back(writeDescSet);

				vkUpdateDescriptorSets(*device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
			}
		}

		ShaderResources GetShaderResource(const std::string& resName) const
		{
			for(const auto& res : resources)
			{
				if(res.name == resName)
					return res;
			}

			Assert(false, "Failed to find shader resource in descriptor set");
			return ShaderResources{};
		}
		VkDescriptorSet* Get(uint32_t offset) { return &sets[offset]; }

		VkDescriptorPool GetPool() { return pool; }

		void* GetResource(const std::string& name, uint32_t offset) { return buffers[name]->Map(); }

		void SetResource(const std::string& name, void* data, const size_t size, const size_t offset) { buffers[name]->Load(data, size, std::max(size * offset, 256 * offset)); }


		void Clear()
		{
			for (auto& val : buffers) { delete val.second; }
			for (auto& val : images) { delete val.second; }
			for (auto& val : samplers) { delete val.second; }
		}

	private:
		uint32_t framesInFlight;
		std::vector<ShaderResources> resources;
		VkDevice* device;
		Memory::Allocator* allocator;
		std::unordered_map<std::string, Memory::Buffer*> buffers;
		std::unordered_map<std::string, Image*> images;
		std::unordered_map<std::string, Sampler*> samplers;

		std::vector<VkDescriptorSet> sets;
		VkDescriptorPool pool;
	};
}

namespace std
{
	template<> struct hash<Renderer::DescriptorSetKey>
	{
		size_t operator()(const Renderer::DescriptorSetKey& s) const noexcept
		{
			return hash<uint64_t>{}(reinterpret_cast<uint64_t>(s.program));
		}
	};
}

namespace Renderer
{

	class DescriptorSetCache : public Cache<DescriptorSetBundle, DescriptorSetKey>
	{
	private:
		VkDevice* device;
		Memory::Allocator* allocator;

	public:
		void BuildCache(VkDevice* device, Memory::Allocator* allocator, uint32_t framesInFlight)
		{
			this->device = device;
			this->allocator = allocator;
			this->framesInFlight = framesInFlight;
		}

		// we can actually build the buffer from what is known in the shader, in this case, we allow our descriptor bundle to also manage the buffer
		void WriteBuffer(DescriptorSetKey& key, const std::string& resName)
		{
			auto descBundle = Get(key);

			descBundle->WriteBuffer(resName);
		}

		void WriteBuffer(DescriptorSetKey& key, const std::string& resName, Memory::Buffer* buffer)
		{
			auto descBundle = Get(key);

			descBundle->WriteBuffer(resName, buffer);
		}

		void WriteSampler(DescriptorSetKey& key, const std::string& resName, Image* image, Sampler* sampler, VkImageLayout layout)
		{
			auto descBundle = Get(key);

			descBundle->WriteSampler(resName, image, sampler, layout);
		}

		void WriteStorageImage(DescriptorSetKey& key, const std::string& resName, Image* image)
		{
			auto descBundle = Get(key);
			
			//descBundle->WriteImage(resName, image, layout);
		}

		template <typename T>
		T* GetResource(const DescriptorSetKey& key, std::string resName)
		{
			auto descSet = Get(key);

			return static_cast<T*>(descSet->GetResource(resName, currentFrame));
		}

		void SetResource(const DescriptorSetKey& key, std::string resName, void* data, size_t size)
		{
			auto descSet = Get(key);

			descSet->SetResource(resName, data, size, currentFrame);
		}

		void BindDescriptorSet(VkCommandBuffer buffer, VkPipelineBindPoint bindPoint, const DescriptorSetKey& key)
		{
			auto descSet = Get(key);

			vkCmdBindDescriptorSets(buffer, bindPoint, key.program->getPipelineLayout(), 0, 1, descSet->Get(currentFrame), static_cast<uint32_t>(key.program->getDynOffsets().size()), key.program->getDynOffsets().data());
		}

		DescriptorSetBundle* Get(const DescriptorSetKey& key) override
		{
			auto& descriptorSet = cache[key];
			if (!descriptorSet)
			{
				descriptorSet = new DescriptorSetBundle(device, allocator, key, framesInFlight);
			}
			return descriptorSet;
		}

		bool Add(const DescriptorSetKey& key) override
		{
			if (cache.find(key) != cache.end()) return false;

			cache.emplace(key, new DescriptorSetBundle(device, allocator, key, framesInFlight));

			return true;
		}

	private:

		void ClearEntry(DescriptorSetBundle* set) override
		{
			vkDestroyDescriptorPool(*device, set->GetPool(), nullptr);
			set->Clear();
		}
	};

}

