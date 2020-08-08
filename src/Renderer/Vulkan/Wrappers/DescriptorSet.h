#pragma once
#include <vector>
#include <vulkan.h>

#include "../../../Utils/Logging.h"
#include "../../Resources/Buffer.h"
#include "../../Resources/Shader.h"
#include <glm/gtc/matrix_transform.hpp>

#include "../../Resources/Image.h"
#include "../../Resources/Sampler.h"


namespace Renderer
{
	struct DescriptorSetKey
	{
		ShaderProgram* program;

		bool operator <(const DescriptorSetKey& other) const { return program < other.program; }
	};

	class DescriptorSetBundle
	{
		public:
			DescriptorSetBundle(VkDevice* device, VmaAllocator* allocator, DescriptorSetKey key, uint32_t framesInFlight) : device(device), allocator(allocator), resources(key.program->getResources()), framesInFlight(framesInFlight)
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

			void writeBuffer(const std::string& resName)
			{
				ShaderResources res;
				for (auto resource : resources)
					if (resource.name == resName)
					{
						res = resource;
						break;
					}

				switch (res.type)
				{
					case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: writeBuffer(resName, new Buffer(allocator, 256U * framesInFlight, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU));
						return;
					case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: writeBuffer(resName, new Buffer(allocator, 256U * framesInFlight, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY));
				}
			}

			void writeBuffer(const std::string& resName, Buffer* buffer)
			{
				buffers.emplace(resName, buffer);

				ShaderResources res;
				for (auto resource : resources)
					if (resource.name == resName)
					{
						res = resource;
						break;
					}


				if (buffer->getSize() < res.size * framesInFlight) { buffer->reSize(res.size * framesInFlight); }

				for (uint32_t i = 0; i < framesInFlight; i++)
				{
					std::vector<VkWriteDescriptorSet> writeSets;

					VkDescriptorBufferInfo descBufferInfo = {};
					descBufferInfo.buffer = buffer->getBuffer();
					descBufferInfo.offset = std::max(i * 256U, i * res.size);
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

			void writeSampler(const std::string& resName, Image* image, Sampler* sampler, VkImageLayout layout)
			{
				ShaderResources res;
				for (auto resource : resources)
					if (resource.name == resName)
					{
						res = resource;
						break;
					}

				VkDescriptorImageInfo imageInfo = {};
				imageInfo.imageView = image->getView();
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

			VkDescriptorSet* get(uint32_t offset) { return &sets[offset]; }

			VkDescriptorPool getPool() { return pool; }

			void* getResource(const std::string& name, uint32_t offset) { return buffers[name]->map(); }

			void setResource(const std::string& name, void* data, const size_t size, const size_t offset) { buffers[name]->load(data, size, std::max(size * offset, 256 * offset)); }


			void clear()
			{
				for (auto& val : buffers) { delete val.second; }
				for (auto& val : images) { delete val.second; }
				for (auto& val : samplers) { delete val.second; }
			}

		private :
			uint32_t framesInFlight;
			std::vector<ShaderResources> resources;
			VkDevice* device;
			VmaAllocator* allocator;
			std::unordered_map<std::string, Buffer*> buffers;
			std::unordered_map<std::string, Image*> images;
			std::unordered_map<std::string, Sampler*> samplers;

			std::vector<VkDescriptorSet> sets;
			VkDescriptorPool pool;
	};
}
