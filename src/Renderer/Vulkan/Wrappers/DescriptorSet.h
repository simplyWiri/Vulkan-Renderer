#pragma once
#include <vector>
#include <vulkan.h>
#include "../../../Utils/Logging.h"
#include "../../Resources/Buffer.h"
#include "../../Resources/Shader.h"
#include <glm/gtc/matrix_transform.hpp>

#include "../../Resources/Image.h"
#include "../../Resources/ImageView.h"
#include "../../Resources/Sampler.h"


namespace Renderer
{
	struct DescriptorSetKey
	{
		ShaderProgram program;
		//std::vector<ShaderResources> resources;
		//VkDescriptorSetLayout dLayout;

		bool operator <(const DescriptorSetKey& other) const { return program < other.program; }
	};

	class DescriptorSetBundle
	{
		public:
			DescriptorSetBundle(VkDevice* device, VmaAllocator* allocator, DescriptorSetKey key, uint32_t framesInFlight)
			{
				std::vector<VkDescriptorPoolSize> poolSizes;

				// initialise our pools
				for (const auto& resource : key.program.getResources())
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

				std::vector<VkDescriptorSetLayout> layouts(framesInFlight, key.program.getDescriptorLayout());

				VkDescriptorSetAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.descriptorPool = pool;
				allocInfo.descriptorSetCount = framesInFlight;
				allocInfo.pSetLayouts = layouts.data();

				sets.resize(framesInFlight);
				success = vkAllocateDescriptorSets(*device, &allocInfo, sets.data());
				Assert(success == VK_SUCCESS, "Failed to allocator descriptor sets");

				// get size of buffer upfront
				createResources(key.program.getResources(), allocator, framesInFlight);

				// update our descriptor sets and create required resources
				for (uint32_t i = 0; i < framesInFlight; i++)
				{
					std::vector<VkWriteDescriptorSet> writeSets;
					for (const auto& res : key.program.getResources())
					{
						VkDescriptorBufferInfo descBufferInfo = {};
						descBufferInfo.buffer = buffers[res.name]->getBuffer();
						descBufferInfo.offset = i * 256;
						// buffer looks like [0 -> res.size, res.size -> res.size * 2] [firstFrameInFlight, secondFrameInFlight]
						descBufferInfo.range = res.size;

						VkWriteDescriptorSet writeDescSet = {};
						writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						writeDescSet.dstSet = sets[i];
						writeDescSet.dstBinding = res.binding;
						writeDescSet.dstArrayElement = 0;
						writeDescSet.descriptorType = res.type;
						writeDescSet.descriptorCount = res.descriptorCount;
						writeDescSet.pBufferInfo = &descBufferInfo;

						writeSets.push_back(writeDescSet);
					}
					vkUpdateDescriptorSets(*device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
				}
			}

			VkDescriptorSet* get(uint32_t offset) { return &sets[offset]; }
			VkDescriptorPool getPool() { return pool; }

			void* getResource(const std::string& name, uint32_t offset) { return buffers[name]->map(); }

			void setResource(const std::string& name, void* data, const size_t size, const size_t offset) { buffers[name]->load(data, size, 256 * offset); }

			void writeSet(const std::string& name) { }

			void clear()
			{
				for (auto& val : buffers) { delete val.second; }
				for (auto& val : images) { delete val.second; }
				for (auto& val : samplers) { delete val.second; }
			}

		private:

			std::unordered_map<std::string, Buffer*> buffers;
			std::unordered_map<std::string, Image*> images;
			std::unordered_map<std::string, Sampler*> samplers;

			void createResources(std::vector<ShaderResources> resources, VmaAllocator* allocator, uint32_t framesInFlight)
			{
				for (const auto& resource : resources)
				{
					auto& name = resource.name;

					switch (resource.type)
					{
						case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: buffers.emplace(name, new Buffer(allocator, 256 * framesInFlight, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU));
							break;
					}
				}
			}

			std::vector<VkDescriptorSet> sets;
			VkDescriptorPool pool;
	};
}
