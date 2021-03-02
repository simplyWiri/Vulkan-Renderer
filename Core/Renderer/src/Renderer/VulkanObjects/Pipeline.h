#pragma once
#include <vector>
#include <memory>

#include "volk/volk.h"

#include "Renderer/Resources/Vertex.h"
#include "Cache.h"

namespace Renderer
{
	class ShaderProgram;
	class Shader;
	enum class ShaderType;

	struct DepthSettings
	{
		VkCompareOp depthFunc;
		bool writeEnable;

		static DepthSettings DepthTest();
		static DepthSettings Disabled();

		bool operator ==(const DepthSettings& other) const;
	};

	struct BlendSettings
	{
		VkPipelineColorBlendAttachmentState blendState;

		static VkPipelineColorBlendAttachmentState createColorAttachmentState(VkColorComponentFlags f, VkBool32 b, VkBlendOp a, VkBlendOp c, VkBlendFactor sc, VkBlendFactor dc);
		static VkPipelineColorBlendAttachmentState createColorAttachmentState(VkColorComponentFlags f, VkBool32 b);
		static BlendSettings Opaque();
		static BlendSettings Add();
		static BlendSettings Mixed();
		static BlendSettings AlphaBlend();

		bool operator ==(const BlendSettings& other) const;
	};

	struct GraphicsPipelineKey
	{
		ShaderProgram* program;
		VkRenderPass renderpass;
		VkExtent2D extent;
		DepthSettings depthSetting;
		VertexAttributes vertexAttributes;
		std::vector<BlendSettings> blendSettings;
		VkPrimitiveTopology topology;

		bool operator ==(const GraphicsPipelineKey& other) const;
	};

	struct ComputePipelineKey
	{
		ShaderProgram* program;

		bool operator ==(const ComputePipelineKey& other) const { return program == other.program; }
	};
}

namespace std
{
	template <>
	struct hash<Renderer::GraphicsPipelineKey>
	{
		size_t operator()(const Renderer::GraphicsPipelineKey& s) const noexcept
		{
			size_t h1 = hash<uint32_t>{}(s.extent.width);
			h1 = h1 ^ (hash<uint32_t>{}(s.extent.height) << 1);
			h1 = h1 ^ (hash<uint64_t>{}(reinterpret_cast<uint64_t>(s.program)) << 1);
			h1 = h1 ^ (hash<Renderer::VertexAttributes>{}(s.vertexAttributes) << 1);
			return h1;
		}
	};

	template <>
	struct hash<Renderer::ComputePipelineKey>
	{
		size_t operator()(const Renderer::ComputePipelineKey& s) const noexcept { return hash<uint64_t>{}(reinterpret_cast<uint64_t>(s.program)); }
	};
}

namespace Renderer
{
	class Pipeline
	{
	private:
		VkDevice* device;
		ShaderProgram* program;

		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;
		std::vector<VkShaderModule> shaderModules;
		std::vector<VkPushConstantRange> pushConstants;
		VkDescriptorSetLayout descriptorSetLayout;


	public:
		Pipeline(VkDevice* device, GraphicsPipelineKey key);
		Pipeline(VkDevice* device, ComputePipelineKey key);

		~Pipeline() { vkDestroyPipeline(*device, pipeline, nullptr); }

		operator VkPipeline() { return pipeline; }

		const VkPipeline& GetPipeline() { return pipeline; }
		const VkPipelineLayout& GetLayout() { return pipelineLayout; }
		const VkDescriptorSetLayout& GetDescriptorLayout() { return descriptorSetLayout; }

	private:
		VkShaderModule CreateShaderModule(Shader* shader);
		VkPipelineShaderStageCreateInfo CreateShaderInfo(VkShaderModule shaderModule, ShaderType type);
	};

	class GraphicsPipelineCache : public Cache<Pipeline, GraphicsPipelineKey>
	{
	private:
		VkDevice* device;

	public:
		void BuildCache(VkDevice* device) { this->device = device; }

		void BindGraphicsPipeline(VkCommandBuffer buffer, VkRenderPass pass, const VkExtent2D& extent, const VertexAttributes& vertexAttributes, const DepthSettings& depthSettings, const std::vector<BlendSettings>& blendSettings,
		                          VkPrimitiveTopology topology, ShaderProgram* program);
		void BindGraphicsPipeline(VkCommandBuffer buffer, GraphicsPipelineKey& key);

		Pipeline* Get(const GraphicsPipelineKey& key) override;

		bool Add(const GraphicsPipelineKey& key) override;

		void ClearEntry(Pipeline* pipeline) override { }
	};

	class ComputePipelineCache : public Cache<Pipeline, ComputePipelineKey>
	{
	private:
		VkDevice* device;

	public:
		void BuildCache(VkDevice* device) { this->device = device; }

		void BindComputePipeline(VkCommandBuffer buffer, ComputePipelineKey key);

		Pipeline* Get(const ComputePipelineKey& key) override;

		bool Add(const ComputePipelineKey& key) override;

		void ClearEntry(Pipeline* pipeline) override { }
	};
}
