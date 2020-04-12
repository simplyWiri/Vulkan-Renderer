#pragma once
#include "Shader.h"
#include "vulkan.h"

namespace Renderer {

	class ShaderProgram
	{
	public:
		ShaderProgram(const std::vector<Shader*>& shaders)
		{
			this->shaders = shaders;
		}

		void getResources(VkDevice* device, VkDescriptorSetLayout& layouts, std::vector<VkPushConstantRange>& pushConstants)
		{
			std::vector<VkDescriptorSetLayoutBinding> bindings;
			for (const Shader* shader : shaders) {
				for (const auto& resource : shader->getResources) {
					if (resource.type == VK_DESCRIPTOR_TYPE_MAX_ENUM) { // push constant (ref. Shader.cpp ~ line 400)
						VkPushConstantRange range;
						range.offset = resource.offset;
						range.size = resource.size;
						range.stageFlags = resource.flags;
						pushConstants.push_back(range);
						continue;
					}

					VkDescriptorSetLayoutBinding binding = {};
					binding.binding = resource.binding;
					binding.descriptorCount = resource.descriptorCount;
					binding.descriptorType = resource.type;
					binding.stageFlags = resource.flags;

					bindings.push_back(binding);
				}
			}

			VkDescriptorSetLayoutCreateInfo desclayout = {};
			desclayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			desclayout.bindingCount = static_cast<uint32_t>(bindings.size());
			desclayout.pBindings = bindings.data();

			vkCreateDescriptorSetLayout(*device, &desclayout, nullptr, &layouts);
		}

	private:
		std::vector<Shader*> shaders;
	};

}

