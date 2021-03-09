#include <unordered_set>
#include <algorithm>

#include "GraphBuilder.h"

#include "Renderer/Core.h"
#include "Utils/Logging.h"
#include "Renderer/Memory/Allocator.h"


namespace Renderer::RenderGraph
{
	GraphBuilder::PassDesc::PassDesc(const std::string& name, GraphBuilder* graph, uint32_t passId, QueueType queue) : name(name), graph(graph), passId(passId), queueIndex(queue) { }

	bool GraphBuilder::PassDesc::WritesTo(const std::string& name)
	{
		for (const auto& res : writtenResources) { if (res == name) return true; }
		return false;
	}

	bool GraphBuilder::PassDesc::ReadsFrom(const std::string& name)
	{
		for (const auto& res : readResources) { if (res == name) return true; }
		return false;
	}

	GraphBuilder::PassDesc& GraphBuilder::PassDesc::ReadBuffer(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags accessFlags)
	{
		const Access access = { this->name, flags, accessFlags | VK_ACCESS_SHADER_READ_BIT };

		auto& res = graph->GetBuffer(name);
		res.AccessedBy(access);

		readResources.emplace(name);

		return *this;
	}

	GraphBuilder::PassDesc& GraphBuilder::PassDesc::ReadImage(const std::string& name, VkPipelineStageFlags flags, VkImageLayout expectedLayout, VkAccessFlags accessFlags)
	{
		const Access access = { this->name, flags, accessFlags | VK_ACCESS_SHADER_READ_BIT, expectedLayout };

		auto& res = graph->GetImage(name);
		res.AccessedBy(access);

		readResources.emplace(name);

		return *this;
	}

	GraphBuilder::PassDesc& GraphBuilder::PassDesc::WriteBuffer(const std::string& name, VkPipelineStageFlags flags, VkAccessFlags accessFlags, const BufferInfo info)
	{
		const Access access = { this->name, flags, accessFlags | VK_ACCESS_SHADER_WRITE_BIT };

		auto& res = graph->GetBuffer(name);
		res.SetInfo(info);
		res.AccessedBy(access);

		writtenResources.emplace(name);

		return *this;
	}

	GraphBuilder::PassDesc& GraphBuilder::PassDesc::WriteImage(const std::string& name, VkPipelineStageFlags flags, VkAttachmentLoadOp loadOp, VkAccessFlags accessFlags, const ImageInfo info)
	{
		const Access access = { this->name, flags, accessFlags | VK_ACCESS_SHADER_WRITE_BIT, info.layout, loadOp };

		auto& res = graph->GetImage(name);
		res.SetInfo(info);
		res.AccessedBy(access);

		writtenResources.emplace(name);

		return *this;
	}

	GraphBuilder::PassDesc& GraphBuilder::PassDesc::WriteToBackbuffer(VkAttachmentLoadOp loadOp)
	{
		const Access access = { name, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, loadOp };

		auto& res = graph->GetImage(graph->GetBackBuffer());
		res.AccessedBy(access);

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

		Assert(false, "Trying to retrieve resource which does not exist.");
	}

	void GraphBuilder::CreateAdjacencyList(std::vector<std::unordered_set<uint32_t>>& adjacencyList)
	{
		// We want to build a list of passes which consume resources written to by a given pass.
		// allowing us to determine read after write dependencies within our graph

		for (int i = 0; i < passDescriptions.size(); i++)
		{
			const auto& pass = passDescriptions[i];
			const auto& writtenResources = pass.writtenResources;

			auto& passAdjacentIndices = adjacencyList[i];

			for (int j = 0; i < passDescriptions.size(); i++)
			{
				if (i == j) continue; // Do not evaluate self

				const auto& otherPass = passDescriptions[j];
				const auto& readResources = otherPass.readResources;

				// Handling multiple writes.
				for(const auto& writtenResource : writtenResources)
				{
					auto& resource = GetResource(writtenResource);

					if(resource.GetWrites().size() == 1) continue;

					if(resource.OrderPassesByWrites(pass.name, otherPass.name) == otherPass.name)
					{
						passAdjacentIndices.emplace(otherPass.GetPassId());
						break;
					}
				}
				

				for (const auto& readResource : readResources)
				{
					if (!writtenResources.contains(readResource)) continue;

					passAdjacentIndices.emplace(otherPass.GetPassId());
					
					// We do not need duplicate entries in the case of multiple reads 
					break;	
				}
			}
		}
	}

	std::vector<uint32_t> GraphBuilder::TopologicalSort(std::vector<std::unordered_set<uint32_t>>& adjacencyList)
	{
		// Using the previously created adjacency list, we aim to create a sorted
		// list wherein no pass (A) is before another pass (B) which writes to a resource
		// that (A) reads

		std::vector<bool> visited(passDescriptions.size(), false);
		std::vector<uint32_t> sortedPassOrder;

		for (int i = 0; i < passDescriptions.size(); i++)
		{
			// Dependent nodes have already been visited
			if (visited[i]) continue;

			DepthFirstSearch(i, visited, sortedPassOrder, adjacencyList);
		}

		std::reverse(sortedPassOrder.begin(), sortedPassOrder.end());

		return sortedPassOrder;
	}

	void GraphBuilder::DepthFirstSearch(int currentIndex, std::vector<bool>& visited, std::vector<uint32_t>& sortedPassOrder, std::vector<std::unordered_set<uint32_t>>& adjacencyList)
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

	// Sorted order is a list of integers which correspond to the current idx of the pass in the `passDescriptions` array
	// After this function, the passId for a pass *will likely change* this is something to fix in the future.
	void GraphBuilder::DependencyLevelSort(const std::vector<uint32_t>& sortedOrder, std::vector<std::unordered_set<uint32_t>>& adjacencyList)
	{
		// first entry is the pass idx, second is the distance.
		std::vector<std::pair<int, int>> distances(sortedOrder.size(), { 0, 0 });

		for (const auto& pass : sortedOrder)
		{
			auto& [passIdx, passDistance] = distances[pass];
			passIdx = pass;

			const auto nodeDistance = passDistance + 1;
			for (const auto& adj : adjacencyList[pass])
			{
				if (distances[adj].second < nodeDistance) 
					distances[adj].second = nodeDistance;
			}
		}

		// We now sort by dependency level
		std::sort(distances.begin(), distances.end(), [](const auto& a, const auto& b) { return a.second < b.second; });

		std::vector<PassDesc> copyPasses;

		for (auto& [passIndex, passDependencyLevel] : distances)
		{
			auto& passDesc = passDescriptions[passIndex];
			passDesc.dependencyGraphIndex = passDependencyLevel;

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

		Log("todo");
	}

	void GraphBuilder::CreateGraph(RenderGraph* graph)
	{
		// Create Adjacency list
		auto adjacencyList = std::vector<std::unordered_set<uint32_t>>(passDescriptions.size());
		CreateAdjacencyList(adjacencyList);

		// Sort
		const auto sortedOrder = TopologicalSort(adjacencyList);

		// Sort w/ Dependency Levels
		DependencyLevelSort(sortedOrder, adjacencyList);

		// todo when we have an example which could make use of resource aliasing. ping pong blur or something similar.
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

		std::vector<std::string> sortedOrder;
		for(auto& pass : passDescriptions) sortedOrder.emplace_back(pass.name);

		for (auto& passDesc : passDescriptions)
		{
			std::vector<ImageResource*> frameBufferImages;

			for (const auto& resKey : passDesc.readResources)
			{
				auto& res = GetResource(resKey);

				// Not sure when this could happen outside of a misconfiguration
				if (res.GetWrites().empty()) 
				{
					Assert(false, "Error, resource which is never written to");
					continue;
				}
				for (auto& access : res.GetWrites())
				{
					auto& writingPass = passDescriptions[nameToPass[access.passAlias]];

					passDesc.syncs.emplace_back(SynchronisationRequirement{ &res, passDesc.queueIndex, writingPass.queueIndex, writingPass.passId, passDesc.passId });
				}
			}

			std::string depthAttach;

			for (const auto& resKey : passDesc.writtenResources)
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

						if (!res.GetReads().empty()) info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

						for (auto i = 0; i < framesInFlight; i++)
						{
							resourceImages[i] = allocator->AllocateImage(extent, info.format, info.usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
						}

						auto imageIndex = static_cast<uint32_t>(graph->images.size());
						graph->images.emplace_back(std::move(resourceImages));
						graph->nameToResource.emplace(resKey, imageIndex);
					}

					auto usage = info.usage;

					// Presumably compute, this is not used as a framebuffer image.
					if (usage == VK_IMAGE_USAGE_STORAGE_BIT) continue;

					auto optionalWrite = res.WrittenByPass(passDesc.name);
					auto& write = optionalWrite.value();
					auto sortedAccesses = res.SortedAccesses(sortedOrder);
					auto sortedAccessesSize = sortedAccesses.size();
					
					auto initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, finalLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;

					if(sortedAccessesSize > 1)
					{
						int i = 0;
						for(; i < sortedAccessesSize; i++)
						{
							if(sortedAccesses[i] == write) break;
						}
						
						if(i != 0) initialLayout = write.expectedLayout;
						if(i != sortedAccessesSize - 1) finalLayout = sortedAccesses[i+1].expectedLayout;
					}
					
					if(res.name == backBuffer.name && write == sortedAccesses.back()) finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
					
					auto attachment = AttachmentDesc{ info.format, write.loadOp, {0,0,0,0}, initialLayout, finalLayout };

					// We assume the image is either a depth attachment or a color attachment.. works for now
					if (info.format == VK_FORMAT_D32_SFLOAT)
					{
						attachment.SetDepthClear(); // Add the 1.0 -> 0.0 clear value for the depth attachment.
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
			if (!depthAttach.empty()) passDesc.framebufferImageAttachments.emplace_back(depthAttach);
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
			pass->executes.emplace_back(std::move(passDesc.execute));

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

			//for (auto& sync : passDesc.syncs)
			//{
			//	auto& imgRes = reinterpret_cast<ImageResource&>(*sync.resourceToSync);
			//	auto& res = graph->GetImage(imgRes.name);

			//	std::vector<VkImageMemoryBarrier> barriers;

			//	pass->imageBarriers.resize(framesInFlight);

			//	for (auto i = 0; i < framesInFlight; i++)
			//	{
			//		VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			//		barrier.image = res[i]->GetResourceHandle();
			//		barrier.oldLayout = imgRes.info.layout;
			//		auto expectedLayout = imgRes.reads.front().layout;
			//		barrier.newLayout = expectedLayout == VK_IMAGE_LAYOUT_UNDEFINED ? imgRes.info.layout : expectedLayout;

			//		barrier.srcAccessMask = imgRes.writes.front().access;
			//		barrier.dstAccessMask = imgRes.reads.front().access;

			//		barrier.srcQueueFamilyIndex = passDescriptions[imgRes.writes.front().passId].GetQueueType() == QueueType::Graphics ? graphics.queueFamilyIndex : compute.queueFamilyIndex;
			//		barrier.dstQueueFamilyIndex = passDescriptions[imgRes.reads.front().passId].GetQueueType() == QueueType::Graphics ? graphics.queueFamilyIndex : compute.queueFamilyIndex;

			//		barrier.subresourceRange = res[i]->GetSubresourceRange();
			//		pass->imageBarriers[i].emplace_back(barrier);
			//	}
			//}


			graph->renderPasses.emplace_back(std::move(pass));
		}

		// Need to initialise after all resources have been created.
		for (const auto& passDesc : passDescriptions) { if (passDesc.initialise) passDesc.initialise(graph); }
	}
}
