#pragma once
#include "vulkan.h"
#include "..\..\Resources\Shader.h" /* Accessing Shader Functionality */
#include <vector>

namespace Renderer {
	struct Context;

	struct DepthSettings
	{
		static VkPipelineDepthStencilStateCreateInfo DepthTest();
		static VkPipelineDepthStencilStateCreateInfo Disabled();
	};

	struct BlendSettings
	{
		static VkPipelineColorBlendAttachmentState Opaque();
		static VkPipelineColorBlendAttachmentState Add();
		static VkPipelineColorBlendAttachmentState Mixed();
		static VkPipelineColorBlendAttachmentState AlphaBlend();
	};

	struct Pipeline
	{
	public:
		void setContext(Context* c) { this->context = c; }
		VkPipeline& getPipeline() { return this->pipeline; }
		VkPipelineLayout& getLayout() { return this->layout; }

		void setResources(std::vector<ShaderResources> r) { createLayout(r); }
		inline VkDescriptorSetLayout& getDescriptorLayout() { return this->descriptorSetLayout; }

		bool createLayout(std::vector<ShaderResources> r);

	private:
		Context* context;
		VkPipeline pipeline;
		VkPipelineLayout layout;

		std::vector<VkPushConstantRange> pushConstants;
		VkDescriptorSetLayout descriptorSetLayout;
	};

	namespace Wrappers {
	}
}
