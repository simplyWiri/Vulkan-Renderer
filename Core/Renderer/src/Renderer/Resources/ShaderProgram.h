#pragma once
#include <vector>

#include "Shader.h"

namespace Renderer
{
	class ShaderProgram
	{
	public:
		ShaderProgram() {}

		ShaderProgram(VkDevice* device, std::vector<Shader*> shaders) : device(device)
		{
			this->shaders = shaders;

			for (auto shader : shaders)
			{
				if (shader->getStatus() == ShaderStatus::Uninitialised)
				{
					shader->compileGLSL();
					shader->reflectSPIRV();
				}

				auto res = shader->getResources();
				for(auto& r : res)
					shaderResources.emplace_back(r);
				
				ids.emplace_back(shader->getId());
			}
		}

		~ShaderProgram()
		{
			vkDestroyDescriptorSetLayout(*device, dLayout, nullptr);
			vkDestroyPipelineLayout(*device, pLayout, nullptr);
		}

		bool operator ==(const ShaderProgram& other) const { return ids == other.ids; }

		void InitialiseResources(VkDevice* device)
		{
			if (initialised) return;

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

					bindings.push_back(binding);

					if (resource.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC || resource.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) { dynOffsets.emplace_back(0); }
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

		VkPipelineLayout getPipelineLayout() const { return pLayout; }
		VkDescriptorSetLayout getDescriptorLayout() const { return dLayout; }

		const std::vector<uint32_t>& getDynOffsets() { return dynOffsets; }
		const std::vector<uint32_t>& getIds() const { return ids; }

	private:
		bool initialised = false;

		VkDevice* device;
		VkPipelineLayout pLayout;
		VkDescriptorSetLayout dLayout;

		std::vector<Shader*> shaders;

		std::vector<ShaderResources> shaderResources;
		std::vector<uint32_t> dynOffsets;
		std::vector<uint32_t> ids;
	};
}

namespace std
{
	template <>
	struct hash<Renderer::ShaderProgram>
	{
		size_t operator()(const Renderer::ShaderProgram& s) const noexcept
		{
			//std::size_t bindingsSeed = s.getResources().size();
			//for (const auto& i : s.getResources())
			//bindingsSeed ^= hash<Renderer::ShaderResources>{}(i)+0x9e3779b9 + (bindingsSeed << 6) + (bindingsSeed >> 2);

			size_t seed = s.getIds().size();
			for (const auto& i : s.getIds()) seed ^= hash<uint32_t>{}(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

			return seed;
		}
	};
}
