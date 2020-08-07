#pragma once
#include "vulkan.h"
#include <vector>
#include <memory>

#include "../../Resources/ShaderProgram.h"
#include "../../Resources/Vertex.h"

namespace Renderer
{
	struct DepthSettings
	{
		VkCompareOp depthFunc;
		bool writeEnable;

		static DepthSettings DepthTest()
		{
			DepthSettings settings;
			settings.depthFunc = VK_COMPARE_OP_LESS;
			settings.writeEnable = VK_TRUE;

			return settings;
		}

		static DepthSettings Disabled()
		{
			DepthSettings settings;
			settings.depthFunc = VK_COMPARE_OP_ALWAYS;
			settings.writeEnable = VK_FALSE;

			return settings;
		}

		bool operator <(const DepthSettings& other) const { return std::tie(depthFunc, writeEnable) < std::tie(other.depthFunc, other.writeEnable); }
	};

	struct BlendSettings
	{
		VkPipelineColorBlendAttachmentState blendState;

		// todo - Find default values for color attachment state.
		static VkPipelineColorBlendAttachmentState createColorAttachmentState(VkColorComponentFlags f, VkBool32 b, VkBlendOp a, VkBlendOp c, VkBlendFactor sc, VkBlendFactor dc)
		{
			VkPipelineColorBlendAttachmentState info = {};
			info.blendEnable = b;
			info.alphaBlendOp = a;
			info.colorBlendOp = c;
			info.colorWriteMask = f;
			info.srcColorBlendFactor = sc;
			info.dstColorBlendFactor = dc;
			return info;
		}

		static VkPipelineColorBlendAttachmentState createColorAttachmentState(VkColorComponentFlags f, VkBool32 b)
		{
			VkPipelineColorBlendAttachmentState info = {};
			info.blendEnable = b;
			info.colorWriteMask = f;

			return info;
		}

		static BlendSettings Opaque()
		{
			BlendSettings settings;
			settings.blendState = createColorAttachmentState((VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT), false);
			return settings;
		}

		static BlendSettings Add()
		{
			BlendSettings settings;
			settings.blendState = createColorAttachmentState((VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT), true, VK_BLEND_OP_ADD, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE);
			return settings;
		}

		static BlendSettings Mixed()
		{
			BlendSettings settings;
			settings.blendState = createColorAttachmentState((VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT), true, VK_BLEND_OP_ADD, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
			return settings;
		}

		static BlendSettings AlphaBlend()
		{
			BlendSettings settings;
			settings.blendState = createColorAttachmentState((VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT), true, VK_BLEND_OP_ADD, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
			return settings;
		}

		bool operator <(const BlendSettings& other) const { return std::tie(blendState.blendEnable, blendState.alphaBlendOp, blendState.colorBlendOp, blendState.srcColorBlendFactor, blendState.dstColorBlendFactor) < std::tie(other.blendState.blendEnable, other.blendState.alphaBlendOp, other.blendState.colorBlendOp, other.blendState.srcColorBlendFactor, other.blendState.dstColorBlendFactor); }
	};

	struct GraphicsPipelineKey
	{
		ShaderProgram program;

		VkRenderPass renderpass;
		VkExtent2D extent;
		DepthSettings depthSetting;
		VertexAttributes vertexAttributes;
		std::vector<BlendSettings> blendSettings;
		VkPrimitiveTopology topology;

		bool operator <(const GraphicsPipelineKey& other) const { return std::tie(depthSetting, blendSettings, topology, renderpass, program, extent.width, extent.height, vertexAttributes) < std::tie(other.depthSetting, other.blendSettings, other.topology, other.renderpass, other.program, other.extent.width, other.extent.height, other.vertexAttributes); }
	};

	struct Pipeline
	{
		private:
			VkDevice* device;
			VkPipeline pipeline;
			VkPipelineLayout layout;

			std::vector<VkShaderModule> shaderModules;
			std::vector<VkPushConstantRange> pushConstants;
			VkDescriptorSetLayout descriptorSetLayout;


		public:
			Pipeline(VkDevice* device, GraphicsPipelineKey key)
			{
				Assert(!(device == nullptr || key.renderpass == nullptr || key.extent.width == 0 || key.extent.height == 0 ), "Failed to obtain required information to create the graphics pipeline");

				this->device = device;
				key.program.initialiseResources(device);
				this->layout = key.program.getPipelineLayout();
				this->descriptorSetLayout = key.program.getDescriptorLayout();

				std::vector<VkPipelineShaderStageCreateInfo> shaderCreateInfo = {};

				for (const auto& shader : key.program.getShaders())
				{
					switch (shader->getType())
					{
						case ShaderType::Vertex: case ShaderType::Fragment:
							// this also populates the field in Pipeline/shaderModules
							shaderCreateInfo.push_back(createShaderInfo(createShaderModule(shader), shader->getType()));
							break;
						default: LogError("Tried to register currently unsupported shader type");
					}
				}

				auto bindingDescription = key.vertexAttributes.getBindings();
				auto attributeDescription = key.vertexAttributes.getAttributes();

				VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
				vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				vertexInputCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescription.size());
				vertexInputCreateInfo.pVertexBindingDescriptions = bindingDescription.data();
				vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
				vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescription.data();

				// Input assembly create info stage
				VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = createInputAssemblyState(key.topology, false);

				// Viewport create info stage
				VkViewport viewport = {};
				viewport.x = 0.0f;
				viewport.y = 0.0f;
				viewport.width = static_cast<float>(key.extent.width);
				viewport.height = static_cast<float>(key.extent.height);
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;

				VkRect2D scissor = {};
				scissor.offset = {0, 0};
				scissor.extent = key.extent;

				VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
				viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				viewportCreateInfo.viewportCount = 1;
				viewportCreateInfo.pViewports = &viewport;
				viewportCreateInfo.scissorCount = 1;
				viewportCreateInfo.pScissors = &scissor;

				//// Rasterizer create info stage
				VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = createRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

				//// Multisampling create info stage
				VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
				multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
				multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

				VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = createColorBlendState(1, &key.blendSettings.begin()->blendState);


				VkPipelineDepthStencilStateCreateInfo depthStencil = {};
				depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
				depthStencil.depthTestEnable = VK_TRUE;
				depthStencil.depthWriteEnable = key.depthSetting.writeEnable;
				depthStencil.depthCompareOp = key.depthSetting.depthFunc;
				depthStencil.depthBoundsTestEnable = VK_FALSE;
				depthStencil.stencilTestEnable = VK_FALSE;

				auto dynamicStates = std::vector<VkDynamicState>{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

				VkPipelineDynamicStateCreateInfo dynamicState = {};
				dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
				dynamicState.dynamicStateCount = 2;
				dynamicState.pDynamicStates = dynamicStates.data();

				//// Creating the pipeline, and tethering it to the struct
				VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
				graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
				graphicsPipelineCreateInfo.stageCount = 2;
				graphicsPipelineCreateInfo.pStages = shaderCreateInfo.data();
				graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
				graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
				graphicsPipelineCreateInfo.pViewportState = &viewportCreateInfo;
				graphicsPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
				graphicsPipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
				graphicsPipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
				graphicsPipelineCreateInfo.pDepthStencilState = &depthStencil;
				graphicsPipelineCreateInfo.pDynamicState = &dynamicState;
				graphicsPipelineCreateInfo.layout = key.program.getPipelineLayout();
				// we have pre-baked the layout in the key
				graphicsPipelineCreateInfo.renderPass = key.renderpass;


				auto success = vkCreateGraphicsPipelines(*device, nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline) == VK_SUCCESS;
				Assert(success, "Failed to create graphics pipeline");

				for (auto shaderModule : shaderModules) { vkDestroyShaderModule(*device, shaderModule, nullptr); }
			}

			operator VkPipeline() { return pipeline; }

			VkPipeline& getPipeline() { return this->pipeline; }
			VkPipelineLayout& getLayout() { return this->layout; }

			VkDescriptorSetLayout& getDescriptorLayout() { return this->descriptorSetLayout; }

		private:
			VkShaderModule Pipeline::createShaderModule(Shader* shader)
			{
				VkShaderModuleCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				createInfo.codeSize = shader->getSize();
				createInfo.pCode = shader->getSPV().data();

				VkShaderModule shaderModule;
				auto success = vkCreateShaderModule(*device, &createInfo, nullptr, &shaderModule) == VK_SUCCESS;
				Assert(success, "Failed to create shader module");

				shaderModules.push_back(shaderModule);

				return shaderModule;
			}

			VkPipelineShaderStageCreateInfo Pipeline::createShaderInfo(VkShaderModule module, ShaderType type)
			{
				VkPipelineShaderStageCreateInfo shaderCreateInfo = {};
				shaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderCreateInfo.stage = getFlagBits(type);
				shaderCreateInfo.module = module;
				shaderCreateInfo.pName = "main";

				return shaderCreateInfo;
			}

			VkPipelineInputAssemblyStateCreateInfo createInputAssemblyState(VkPrimitiveTopology t, VkBool32 p)
			{
				VkPipelineInputAssemblyStateCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				info.topology = t;
				info.primitiveRestartEnable = p;

				return info;
			}

			VkPipelineRasterizationStateCreateInfo createRasterizationState(VkPolygonMode m, VkCullModeFlags c, VkFrontFace f)
			{
				VkPipelineRasterizationStateCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
				info.depthClampEnable = VK_FALSE;
				info.rasterizerDiscardEnable = VK_FALSE;
				info.polygonMode = m;
				info.lineWidth = 1.0f;
				info.cullMode = c;
				info.frontFace = f;
				info.depthBiasEnable = VK_FALSE;

				return info;
			}

			VkPipelineColorBlendStateCreateInfo createColorBlendState(uint32_t c, VkPipelineColorBlendAttachmentState* s)
			{
				VkPipelineColorBlendStateCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				info.logicOpEnable = VK_FALSE;
				info.logicOp = VK_LOGIC_OP_COPY;
				info.attachmentCount = c;
				info.pAttachments = s;
				info.blendConstants[0] = 0.0f;
				info.blendConstants[1] = 0.0f;
				info.blendConstants[2] = 0.0f;
				info.blendConstants[3] = 0.0f;

				return info;
			}

			VkPipelineDepthStencilStateCreateInfo createDepthStencilState(VkBool32 w, VkCompareOp c)
			{
				VkPipelineDepthStencilStateCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
				info.depthTestEnable = VK_TRUE;
				info.depthWriteEnable = w;
				info.depthCompareOp = c;
				info.depthBoundsTestEnable = VK_FALSE;
				info.stencilTestEnable = VK_FALSE;

				return info;
			}
	};
}
