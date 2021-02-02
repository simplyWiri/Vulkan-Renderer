#include "DescriptorSet.h"

#include <Tracy.hpp>

#include "../Resources/ShaderProgram.h"
#include "../../Utils/Logging.h"
#include "../Memory/Allocator.h"
#include "../Memory/Buffer.h"
#include "../Memory/Image.h"
#include "../Resources/Sampler.h"

namespace Renderer
{
	bool DescriptorSetKey::operator==(const DescriptorSetKey& other) const { return program == other.program; }


	DescriptorSetBundle::DescriptorSetBundle(VkDevice* device, Memory::Allocator* allocator, DescriptorSetKey key, uint32_t framesInFlight) : framesInFlight(framesInFlight), resources(key.program->getResources()), device(device),
	                                                                                                                                          allocator(allocator)
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

	void DescriptorSetBundle::WriteBuffer(const std::string& resName)
	{
		const auto res = GetShaderResource(resName);
		uint32_t size = res.size;

		size = std::max(256U, size); // 256U is the min alignment req

		switch (res.type)
		{
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: WriteBuffer(resName,
					allocator->AllocateBuffer(size * framesInFlight, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
				break;
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: WriteBuffer(resName, allocator->AllocateBuffer(size * framesInFlight, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
				break;
		}
	}

	void DescriptorSetBundle::WriteBuffer(const std::string& resName, Memory::Buffer* buffer)
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

	void DescriptorSetBundle::WriteSampler(const std::string& resName, Memory::Image* image, Sampler* sampler, VkImageLayout layout)
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

	ShaderResources DescriptorSetBundle::GetShaderResource(const std::string& resName) const
	{
		for (const auto& res : resources) { if (res.name == resName) return res; }

		Assert(false, "Failed to find shader resource in descriptor set");
		return ShaderResources{};
	}

	void* DescriptorSetBundle::GetResource(const std::string& name, uint32_t offset) { return buffers[name]->Map(); }

	void DescriptorSetBundle::SetResource(const std::string& name, void* data, const size_t size, const size_t offset) { buffers[name]->Load(data, size, std::max(size * offset, 256 * offset)); }


	void DescriptorSetBundle::Clear()
	{
		for (auto& val : buffers) { delete val.second; }
		for (auto& val : samplers) { delete val.second; }
	}

	void DescriptorSetCache::BuildCache(VkDevice* device, Memory::Allocator* allocator, uint32_t framesInFlight)
	{
		this->device = device;
		this->allocator = allocator;
		this->framesInFlight = framesInFlight;
	}

	void DescriptorSetCache::WriteBuffer(DescriptorSetKey& key, const std::string& resName)
	{
		auto descBundle = Get(key);

		descBundle->WriteBuffer(resName);
	}

	void DescriptorSetCache::WriteBuffer(DescriptorSetKey& key, const std::string& resName, Memory::Buffer* buffer)
	{
		auto descBundle = Get(key);

		descBundle->WriteBuffer(resName, buffer);
	}

	void DescriptorSetCache::WriteSampler(DescriptorSetKey& key, const std::string& resName, Memory::Image* image, Sampler* sampler, VkImageLayout layout)
	{
		auto descBundle = Get(key);

		descBundle->WriteSampler(resName, image, sampler, layout);
	}

	void DescriptorSetCache::SetResource(const DescriptorSetKey& key, std::string resName, void* data, size_t size)
	{
		auto descSet = Get(key);

		descSet->SetResource(resName, data, size, currentFrame);
	}

	void DescriptorSetCache::BindDescriptorSet(VkCommandBuffer buffer, VkPipelineBindPoint bindPoint, const DescriptorSetKey& key)
	{
		ZoneScopedN("Binding Descriptor Set")

		auto descSet = Get(key);

		vkCmdBindDescriptorSets(buffer, bindPoint, key.program->getPipelineLayout(), 0, 1, descSet->Get(currentFrame), static_cast<uint32_t>(key.program->getDynOffsets().size()), key.program->getDynOffsets().data());
	}

	DescriptorSetBundle* DescriptorSetCache::Get(const DescriptorSetKey& key)
	{
		auto& descriptorSet = cache[key];
		if (!descriptorSet) { descriptorSet = new DescriptorSetBundle(device, allocator, key, framesInFlight); }
		return descriptorSet;
	}

	bool DescriptorSetCache::Add(const DescriptorSetKey& key)
	{
		if (cache.find(key) != cache.end()) return false;

		cache.emplace(key, new DescriptorSetBundle(device, allocator, key, framesInFlight));

		return true;
	}

	void DescriptorSetCache::ClearEntry(DescriptorSetBundle* set)
	{
		vkDestroyDescriptorPool(*device, set->GetPool(), nullptr);
		set->Clear();
	}
}
