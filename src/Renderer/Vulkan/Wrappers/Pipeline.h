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

	struct PipelineSettings
	{
		// Input Assembly Stage
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		bool primitiveRestart = false;

		// Rasterizer
		bool discardEnable = false;
		VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
		float lineWidth = 1.0f;
		VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
		VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		bool depthBias = false;

		// Multisampling - Idk

		// DynamicState - Idk
	};

	struct Pipeline
	{
	public:
		void setContext(Context* c) { this->context = c; }
		VkPipeline& getPipeline() { return this->pipeline; }
		VkPipelineLayout& getLayout() { return this->layout; }

		void setResources(std::vector<ShaderResources> r) { this->resources = r; }
		inline VkDescriptorSetLayout& getDescriptorLayout() { return this->descriptorSetLayout; }

		bool createLayout();

	private:
		Context* context;
		VkPipeline pipeline;
		VkPipelineLayout layout;
		VkPipelineCache cache;

		VkRenderPass* renderpass; /* Renderpass to target */
		std::vector<VkRenderPass>* subpasses; /* Subpasses */

		std::vector<ShaderResources> resources;
		std::vector<VkPushConstantRange> pushConstants;
		VkDescriptorSetLayout descriptorSetLayout;
	};

	namespace Wrappers {
		bool buildGraphicsPipeline(Pipeline& pipeline, PipelineSettings settings, std::initializer_list<Shader*> shaders);
		bool buildComputePipeline(Pipeline& pipeline, PipelineSettings settings, std::initializer_list<Shader*> shaders);
	}

}
