#pragma once
#include <vector>

#include "Shader.h"

namespace Renderer
{

	class ShaderProgram
	{
	public:
		ShaderProgram() {}
		ShaderProgram(std::vector<Shader*> shaders)
		{
			this->shaders = shaders;

			for(auto shader : shaders)
			{
				if(shader->getStatus() == ShaderStatus::Uninitialised)
				{
					shader->compileGLSL();
					shader->reflectSPIRV();
				}
				auto& res = shader->getResources();
				shaderResources.insert(shaderResources.end(), res.begin(), res.end());
			}
		}

		void initialiseResources(VkDevice* device)
		{
			if(initialised) return;
			
			std::vector<VkPushConstantRange> pushConstants;
			std::vector<VkDescriptorSetLayoutBinding> bindings;

			for (const auto& shader : shaders)
			{
				if (shader->getStatus() == ShaderStatus::Uninitialised)
				{
					shader->compileGLSL();
					shader->reflectSPIRV();
				}

				for (const auto& resource : shader->getResources())
				{
					if (resource.type == VK_DESCRIPTOR_TYPE_MAX_ENUM)
					{
						// push constant (ref. Shader.cpp ~ line 400)
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
					binding.pImmutableSamplers = nullptr;

					bindings.push_back(binding);
				}
			}			
			
			VkDescriptorSetLayoutCreateInfo desclayout = {};
			desclayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			desclayout.bindingCount = static_cast<uint32_t>(bindings.size());
			desclayout.pBindings = bindings.data();

			vkCreateDescriptorSetLayout(*device, &desclayout, nullptr, &dLayout);

			VkPipelineLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layoutInfo.setLayoutCount = 1;
			layoutInfo.pSetLayouts = &dLayout;
			layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
			layoutInfo.pPushConstantRanges = pushConstants.data();

			vkCreatePipelineLayout(*device, &layoutInfo, nullptr, &pLayout);

			initialised = true;
		}
		
		std::vector<Shader*> getShaders() const { return shaders; }
		std::vector<ShaderResources> getResources() const { return shaderResources; }

		VkPipelineLayout& getPipelineLayout() { return pLayout; }
		VkDescriptorSetLayout& getDescriptorLayout() { return dLayout; }

	private:
		bool initialised = false;
		
		VkPipelineLayout pLayout;
		VkDescriptorSetLayout dLayout;
		
		
		std::vector<Shader*> shaders;
		std::vector<ShaderResources> shaderResources;
	};
}