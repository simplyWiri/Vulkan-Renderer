#include <unordered_set>

#include "GraphBuilder.h"

#include "Renderer/Core.h"
#include "Utils/Logging.h"
#include "Renderer/Memory/Allocator.h"

namespace Renderer::RenderGraph
{
	GraphBuilder::PassDesc::PassDesc(const std::string& name, GraphBuilder* graph, uint32_t passId, QueueType queue) : name(name), graph(graph), passId(passId), queueIndex(queue) { }

	bool GraphBuilder::PassDesc::WritesTo(const std::string& name)
	{
		for (auto res : writtenResources) { if (res == name) return true; }
		return false;
	}

	bool GraphBuilder::PassDesc::ReadsFrom(const std::string& name)
	{
		for (auto res : readResources) { if (res == name) return true; }
		return false;
	}

	GraphBuilder::PassDesc& GraphBuilder::PassDesc::ReadBuffer(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access)
	{
		const Usage usage = { passId, flags, access, queueIndex };

		auto& res = graph->GetBuffer(name);
		res.ReadBy(usage);

		readResources.emplace(name);

		return *this;
	}

	GraphBuilder::PassDesc& GraphBuilder::PassDesc::ReadImage(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access, VkImageLayout expectedLayout)
	{
		const Usage usage = { passId, flags, access, queueIndex, expectedLayout };

		auto& res = graph->GetImage(name);
		res.ReadBy(usage);

		readResources.emplace(name);

		return *this;
	}

	GraphBuilder::PassDesc& GraphBuilder::PassDesc::WriteBuffer(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access, const BufferInfo& info)
	{
		const Usage usage = { passId, flags, access, queueIndex };

		auto& res = graph->GetBuffer(name);
		res.SetInfo(info);
		res.WrittenBy(usage);

		writtenResources.emplace(name);

		return *this;
	}

	GraphBuilder::PassDesc& GraphBuilder::PassDesc::WriteImage(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags access, const ImageInfo& info)
	{
		const Usage usage = { passId, flags, access, queueIndex };

		auto& res = graph->GetImage(name);
		res.SetInfo(info);
		res.WrittenBy(usage);

		if (info.layout == VK_IMAGE_LAYOUT_UNDEFINED) { res.info.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR; }

		writtenResources.emplace(name);

		return *this;
	}

	GraphBuilder::PassDesc& GraphBuilder::PassDesc::WriteToBackbuffer(VkPipelineStageFlags flags, VkAccessFlags access)
	{
		const Usage usage = { passId, flags, access, queueIndex };

		auto& res = graph->GetImage(graph->GetBackBuffer());
		res.WrittenBy(usage);

		writtenResources.emplace(graph->GetBackBuffer());

		return *this;
	}

	GraphBuilder::PassDesc& GraphBuilder::AddPass(const std::string& name, QueueType type)
	{
		auto val = nameToPass.find(name);

		if (val != nameToPass.end()) return static_cast<PassDesc&>(passDescriptions[val->second]);

		const auto index = static_cast<uint32_t>(passDescriptions.size());
		int queueFamily = 0;

		passDescriptions.emplace_back(PassDesc(name, this, index, type));
		nameToPass[name] = index;

		return static_cast<PassDesc&>(passDescriptions.back());
	}

	ImageResource& GraphBuilder::GetImage(const std::string& name)
	{
		if (name == backBuffer.name) return backBuffer;

		auto val = nameToResource.find(name);

		if (val != nameToResource.end()) return reinterpret_cast<ImageResource&>(*resources[val->second]);

		const auto index = static_cast<uint32_t>(resources.size());
		resources.emplace_back(new ImageResource(name));
		nameToResource[name] = index;

		return reinterpret_cast<ImageResource&>(*resources.back());
	}

	BufferResource& GraphBuilder::GetBuffer(const std::string& name)
	{
		auto val = nameToResource.find(name);

		if (val != nameToResource.end()) return reinterpret_cast<BufferResource&>(*resources[val->second]);

		const auto index = static_cast<uint32_t>(resources.size());
		resources.emplace_back(new BufferResource(name));
		nameToResource[name] = index;

		return reinterpret_cast<BufferResource&>(*resources.back());
	}

	ResourceDescription& GraphBuilder::GetResource(const std::string& name)
	{
		if (name == backBuffer.name) return backBuffer;

		auto val = nameToResource.find(name);

		if (val != nameToResource.end()) return reinterpret_cast<ResourceDescription&>(*resources[val->second]);

		Assert(false, "Trying to retrieve resource which does not exist.")
	}

	void GraphBuilder::CreateAdjacencyList(std::vector<std::vector<uint32_t>>& adjacencyList)
	{
		// We want to build a list of passes which consume resources written to by a given pass.
		// allowing us to determine read after write dependencies within our graph

		for (int i = 0; i < passDescriptions.size(); i++)
		{
			auto& pass = passDescriptions[i];
			auto& adjacentIndices = adjacencyList[i];

			auto& writtenResources = pass.writtenResources;

			for (int j = 0; i < passDescriptions.size(); i++)
			{
				if (i == j) continue; // Do not evaluate self

				auto& otherPass = passDescriptions[j];
				auto& readResources = otherPass.readResources;

				for (const auto& readResource : readResources)
				{
					if (writtenResources.contains(readResource))
					{
						adjacentIndices.emplace_back(otherPass.GetPassId());
						// We do not need duplicate entries for multiple reads 
						break;
					}
				}
			}
		}
	}

	std::vector<uint32_t> GraphBuilder::TopologicalSort(std::vector<std::vector<uint32_t>>& adjacencyList)
	{
		// Using the previously created adjacency list, we aim to create a sorted
		// list wherein no pass (A) is before another pass (B) which writes to a resource
		// that (A) reads

		std::vector<bool> visited(passDescriptions.size(), false);
		std::vector<uint32_t> sortedPassOrder;

		for (int i = 0; i < passDescriptions.size(); i++)
		{
			auto& pass = passDescriptions[i];
			auto& adjacenctIndices = adjacencyList[i];

			// Dependent nodes have already been visited
			if (visited[i]) continue;

			DepthFirstSearch(i, visited, sortedPassOrder, adjacencyList);
		}

		std::reverse(sortedPassOrder.begin(), sortedPassOrder.end());

		return sortedPassOrder;
	}

	void GraphBuilder::DepthFirstSearch(int currentIndex, std::vector<bool>& visited, std::vector<uint32_t>& sortedPassOrder, std::vector<std::vector<uint32_t>>& adjacencyList)
	{
		visited[currentIndex] = true;

		for (auto adjacentPass : adjacencyList[currentIndex])
		{
			// if we have already visited we don't need to check its dependencies
			// because its dependencies should have already been checked.
			if (visited[adjacentPass]) continue;

			DepthFirstSearch(adjacentPass, visited, sortedPassOrder, adjacencyList);
		}

		sortedPassOrder.emplace_back(currentIndex);
	}

	void GraphBuilder::DependencyLevelSort(std::vector<uint32_t> sortedOrder, std::vector<std::vector<uint32_t>>& adjacencyList)
	{
		// first entry is the pass idx, second is the distance.
		std::vector<std::pair<int, int>> distances(sortedOrder.size(), { 0, 0 });

		for (auto& node : sortedOrder)
		{
			distances[node].first = node;

			auto nodeDist = distances[node].second + 1;
			for (auto& adj : adjacencyList[node]) { if (distances[adj].second < nodeDist) distances[adj].second = nodeDist; }
		}

		// We now sort by dependency level
		std::sort(distances.begin(), distances.end(), [](std::pair<int, int>& a, std::pair<int, int> b) { return a.second < b.second; });

		auto copyPasses = std::vector<PassDesc>();

		for (auto& pass : distances)
		{
			auto& passDesc = passDescriptions[pass.first];
			passDesc.dependencyGraphIndex = pass.second;

			auto newId = static_cast<uint32_t>(copyPasses.size());
			// Update ID's
			passDesc.passId = newId;
			nameToPass[passDesc.name] = newId;
			copyPasses.emplace_back(std::move(passDesc));
		}

		// Re-arrange the passDescriptions according to this new sorting order.
		passDescriptions = std::move(copyPasses);
	}


	void GraphBuilder::DetermineResourceLifetimes()
	{
		// We want to arrange resource lifetimes w.r.t to dependency levels, to allow for batching of sync
		// this being a trade-off, we could instead look pass-by-pass to achieve more fine-grain sync

		LogInfo("todo");
	}

	void GraphBuilder::CreateGraph(RenderGraph* graph)
	{
		// Create Adjacency list
		auto adjacencyList = std::vector<std::vector<uint32_t>>(passDescriptions.size());
		CreateAdjacencyList(adjacencyList);

		// Sort
		auto sortedOrder = TopologicalSort(adjacencyList);

		// Sort w/ Dependency Levels
		DependencyLevelSort(sortedOrder, adjacencyList);

		// todo when we have an example which could make use of resource aliasing. ping pong blur or something.
		//DetermineResourceLifetimes();

		// Build

		// Create the required physical images (we hopefully 
		CreatePhysicalResources(graph);

		CullRedundantSynchronisation(graph);

		CreateRenderPasses(graph);
	}

	void GraphBuilder::CreatePhysicalResources(RenderGraph* graph)
	{
		// We can now specify information about the backbuffer.
		backBuffer.info.format = graph->core->GetSwapchain()->GetFormat();
		backBuffer.info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		const auto framesInFlight = graph->core->GetSwapchain()->GetFramesInFlight();
		const auto swapchainExtent = graph->core->GetSwapchain()->GetExtent();
		auto* allocator = graph->core->GetAllocator();

		for (auto& passDesc : passDescriptions)
		{
			std::vector<ImageResource*> frameBufferImages;

			for (auto& resKey : passDesc.readResources)
			{
				auto& res = GetResource(resKey);

				// Not sure when this could happen. 
				if (res.writes.empty()) continue;

				auto& writingPass = passDescriptions[res.writes.front().passId];

				passDesc.syncs.emplace_back(SynchronisationRequirement{ &res, passDesc.queueIndex, writingPass.queueIndex, writingPass.passId, passDesc.passId });
			}

			std::string depthAttach = "";

			for (auto& resKey : passDesc.writtenResources)
			{
				auto& res = GetResource(resKey);

				if (res.type == ResourceDescription::Type::Image)
				{
					auto& imgRes = reinterpret_cast<ImageResource&>(res);
					auto info = imgRes.info;
					// Create the actual image from the information given by the image resource.

					if (resKey != backBuffer.name)
					{
						std::vector<Memory::Image*> resourceImages(framesInFlight);

						VkExtent2D extent;
						if (info.sizeType == ImageInfo::SizeType::Swapchain) { extent = swapchainExtent; }
						else if (info.sizeType == ImageInfo::SizeType::Fixed) { extent = VkExtent2D{ static_cast<uint32_t>(info.size.x), static_cast<uint32_t>(info.size.y) }; }
						else { extent = VkExtent2D{ static_cast<uint32_t>(swapchainExtent.width * static_cast<float>(info.size.x)), static_cast<uint32_t>(swapchainExtent.height * static_cast<float>(info.size.y)) }; }

						if (res.reads.size() != 0) info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

						for (auto i = 0; i < framesInFlight; i++) { resourceImages[i] = allocator->AllocateImage(extent, info.format, info.usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); }

						auto imageIndex = static_cast<uint32_t>(graph->images.size());
						graph->images.emplace_back(std::move(resourceImages));
						graph->nameToResource.emplace(resKey, imageIndex);
					}

					auto usage = imgRes.info.usage;

					// Presumably compute, this is not used as a framebuffer image.
					if (usage == VK_IMAGE_USAGE_STORAGE_BIT) continue;

					auto attachment = AttachmentDesc{ imgRes.info.format, VK_ATTACHMENT_LOAD_OP_CLEAR };

					// We assume the image is either a depth attachment or a color attachment.. works for now
					if (imgRes.info.format == VK_FORMAT_D32_SFLOAT)
					{
						passDesc.depthAttachment = attachment;
						depthAttach = resKey;
					}
					else
					{
						passDesc.colorAttachments.emplace_back(attachment);
						passDesc.framebufferImageAttachments.emplace_back(resKey);
					}
				}
				
			}
			if(depthAttach != "")
				passDesc.framebufferImageAttachments.emplace_back(depthAttach);
		}
	}

	void GraphBuilder::CullRedundantSynchronisation(RenderGraph* graph) { }

	void GraphBuilder::CreateRenderPasses(RenderGraph* graph)
	{
		const auto framesInFlight = graph->core->GetSwapchain()->GetFramesInFlight();
		const auto swapchainExtent = graph->core->GetSwapchain()->GetExtent();

		for (auto& passDesc : passDescriptions)
		{
			auto pass = std::make_unique<Pass>();

			pass->name = passDesc.name;
			pass->renderExtent = swapchainExtent;
			pass->execute = std::move(passDesc.execute);

			// Set our renderpass key
			auto rpKey = RenderpassKey{ passDesc.colorAttachments, passDesc.depthAttachment };
			pass->key = std::move(rpKey);

			pass->views.resize(framesInFlight);

			for (const auto& imgRes : passDesc.framebufferImageAttachments)
			{
				if (imgRes == backBuffer.name) { for (int i = 0; i < framesInFlight; i++) pass->views[i].emplace_back(graph->core->GetSwapchain()->GetImageViews()[i]); }
				else
				{
					auto& res = graph->GetImage(imgRes);
					for (int i = 0; i < framesInFlight; i++) pass->views[i].emplace_back(res[i]->GetView());
				}
			}

			auto graphics = graph->queues.graphics;
			auto compute = graph->queues.compute;

			for (auto& sync : passDesc.syncs)
			{
				auto& imgRes = reinterpret_cast<ImageResource&>(*sync.resourceToSync);
				auto& res = graph->GetImage(imgRes.name);

				std::vector<VkImageMemoryBarrier> barriers;

				pass->imageBarriers.resize(framesInFlight);

				for (auto i = 0; i < framesInFlight; i++)
				{
					VkImageMemoryBarrier2KHR barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
					barrier.image = res[i]->GetResourceHandle();
					barrier.oldLayout = imgRes.info.layout;
					auto expectedLayout = imgRes.reads.front().layout;
					barrier.newLayout = expectedLayout == VK_IMAGE_LAYOUT_UNDEFINED ? imgRes.info.layout : expectedLayout;

					barrier.srcAccessMask = imgRes.writes.front().access;
					barrier.dstAccessMask = imgRes.reads.front().access;

					barrier.srcStageMask = imgRes.writes.front().flags;
					barrier.dstStageMask = imgRes.reads.front().flags;

					barrier.srcQueueFamilyIndex = passDescriptions[imgRes.writes.front().passId].GetQueueType() == QueueType::Graphics ? graphics.queueFamilyIndex : compute.queueFamilyIndex;
					barrier.dstQueueFamilyIndex = passDescriptions[imgRes.reads.front().passId].GetQueueType() == QueueType::Graphics ? graphics.queueFamilyIndex : compute.queueFamilyIndex;

					barrier.subresourceRange = res[i]->GetSubresourceRange();
					pass->imageBarriers[i].emplace_back(barrier);
				}
			}


			graph->renderPasses.emplace_back(std::move(pass));
		}

		// Need to initialise after all resources have been created.
		for (auto passDesc : passDescriptions) { if (passDesc.initialise) passDesc.initialise(graph); }
	}
}
