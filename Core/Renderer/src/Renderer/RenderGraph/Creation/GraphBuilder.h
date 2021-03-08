#pragma once
#include <functional>
#include <unordered_map>
#include <unordered_set>

#include "volk/volk.h"

#include "ResourceDescription.h"
#include "Renderer/RenderGraph/GraphContext.h"
#include "Renderer/VulkanObjects/Swapchain.h"
#include "Renderer/RenderGraph/RenderGraph.h"

namespace Renderer::RenderGraph
{
	enum class QueueType : char;
	class RenderGraph;

	class GraphBuilder
	{
		friend class RenderGraph;

	public:
		struct SynchronisationRequirement
		{
			ResourceDescription* resourceToSync;
			QueueType srcQueue;
			QueueType destQueue;

			uint32_t srcPassId;
			uint32_t dstPassId;
		};

		struct PassDesc
		{
			friend class GraphBuilder;
		private:
			GraphBuilder* graph;

			std::string name;
			uint32_t passId;

			uint32_t dependencyGraphIndex;
			QueueType queueIndex;

			std::unordered_set<std::string> readResources;
			std::unordered_set<std::string> writtenResources;

			std::function<void(RenderGraph*)> initialise;
			std::function<void(VkCommandBuffer, const FrameInfo&, GraphContext& context)> execute;

			bool writesBackbuffer;
			std::vector<AttachmentDesc> colorAttachments;
			AttachmentDesc depthAttachment{ VK_FORMAT_UNDEFINED };

			std::vector<std::string> framebufferImageAttachments;
			std::vector<SynchronisationRequirement> syncs;

			bool WritesTo(const std::string& name);
			bool ReadsFrom(const std::string& name);

		public:
			std::string GetName() const { return name; }
			uint32_t GetPassId() const { return passId; }
			QueueType GetQueueType() const { return queueIndex; }

		public:
			PassDesc(const std::string& name, GraphBuilder* builder, uint32_t passId, QueueType queue);

			PassDesc& ReadBuffer(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags accessFlags = VK_ACCESS_SHADER_READ_BIT);
			PassDesc& ReadImage(const std::string& name, VkPipelineStageFlags flags, VkImageLayout expectedLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR, VkAccessFlags accessFlags = VK_ACCESS_SHADER_READ_BIT);

			PassDesc& WriteBuffer(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags accessFlags, const BufferInfo info);
			PassDesc& WriteImage(const std::string& name, VkPipelineStageFlags flags, VkAttachmentLoadOp loadOp, VkAccessFlags accessFlags, const ImageInfo info);
			PassDesc& WriteToBackbuffer(VkAttachmentLoadOp loadOp);

			PassDesc& SetRecordFunc(std::function<void(VkCommandBuffer, const FrameInfo&, GraphContext& context)> func)
			{
				this->execute = std::move(func);
				return *this;
			}

			PassDesc& SetInitialisationFunc(std::function<void(RenderGraph*)> func)
			{
				this->initialise = std::move(func);
				return *this;
			}
		};

	private:
		ImageResource backBuffer{ "_backBuffer" };

		std::unordered_map<std::string, uint32_t> nameToPass;
		std::vector<PassDesc> passDescriptions;

		std::unordered_map<std::string, uint32_t> nameToResource;
		std::vector<ResourceDescription*> resources;

	public:
		std::string GetBackBuffer() const { return backBuffer.name; }

		PassDesc& AddPass(const std::string& name, QueueType type);

		ResourceDescription& GetResource(const std::string& name);
		ImageResource& GetImage(const std::string& name);
		BufferResource& GetBuffer(const std::string& name);


	public:
		// Stage 1. Organising Data about the RenderGraph

		void CreateAdjacencyList(std::vector<std::unordered_set<uint32_t>>& adjacencyList);
		std::vector<uint32_t> TopologicalSort(std::vector<std::unordered_set<uint32_t>>& adjacencyList);
		void DepthFirstSearch(int currentIndex, std::vector<bool>& visited, std::vector<uint32_t>& sortedPassOrder, std::vector<std::unordered_set<uint32_t>>& adjacencyList);
		void DependencyLevelSort(const std::vector<uint32_t>& sortedOrder, std::vector<std::unordered_set<uint32_t>>& adjacencyList);

		void DetermineResourceLifetimes();

		// Stage 2. Creation of Rendergraph
		void CreateGraph(RenderGraph* graph);

		void CreatePhysicalResources(RenderGraph* graph);
		void CullRedundantSynchronisation(RenderGraph* graph);
		void CreateRenderPasses(RenderGraph* graph);
	};
}
