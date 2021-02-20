#include "PlanetRenderer.h"

#include <vulkan_core.h>
#include "Planet.h"
#include "Renderer/Memory/Allocator.h"
#include "Renderer/Memory/Buffer.h"
#include "Renderer/RenderGraph/GraphContext.h"
#include "Renderer/Resources/Vertex.h"
#include "Renderer/VulkanObjects/DescriptorSet.h"
#include "Renderer/VulkanObjects/Pipeline.h"

namespace World
{
	using namespace Renderer;
	
	PlanetRenderer::PlanetRenderer(Generation::PlanetGenerator* planet, Renderer::Memory::Allocator* alloc)
		: planet(planet), alloc(alloc)
	{
		//this->vertexBuffer = alloc->AllocateBuffer(sizeof(Renderer::Vertex) * 500, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		//this->planetVertexBuffer = alloc->AllocateBuffer(sizeof(Renderer::Vertex) * 2500, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		//this->edgesBuffer = alloc->AllocateBuffer(sizeof(Renderer::Vertex) * 25000, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		auto usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		auto memoryType = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		beachlineBuffer = alloc->AllocateBuffer(sizeof(Vertex), usage, memoryType);
		sweeplineBuffer = alloc->AllocateBuffer(sizeof(Vertex), usage, memoryType);
		sitesBuffer = alloc->AllocateBuffer(sizeof(Vertex), usage, memoryType);
		voronoiEdges = alloc->AllocateBuffer(sizeof(Vertex), usage, memoryType);
		delauneyEdges = alloc->AllocateBuffer(sizeof(Vertex), usage, memoryType);
	}

	PlanetRenderer::~PlanetRenderer()
	{
		delete beachlineBuffer;
		delete sweeplineBuffer;
		delete sitesBuffer;
		delete voronoiEdges;
		delete delauneyEdges;
	}

	void PlanetRenderer::DrawBeachline(VkCommandBuffer buffer, GraphContext& context, const VertexAttributes& vert, const DescriptorSetKey& descriptorKey)
	{
		if(planet->beach.Count() < 3) return;

		context.GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetDefaultRenderpass(), context.GetSwapchainExtent(), vert, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, descriptorKey.program);

		auto* current = planet->beach.First();

		std::vector<Vertex> vertices;
		do
		{
			vertices.emplace_back(Vertex{ current->element->site->position, { .5 , .5 ,0 }});
			current = current->Successor();
		} while (current != nullptr);

		vertices.push_back(vertices.front());

		if(beachlineBuffer->GetSize() < sizeof(Vertex) * vertices.size() ) beachlineBuffer = Memory::Buffer::Resize(alloc, beachlineBuffer, sizeof(Vertex) * vertices.size() * 2);
		beachlineBuffer->Load(static_cast<void*>(vertices.data()), sizeof(Vertex) * vertices.size());

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &beachlineBuffer->GetResourceHandle(), offsets);
		vkCmdDraw(buffer, vertices.size(), 1, 0, 0);
	}

	void PlanetRenderer::DrawSweepline(VkCommandBuffer buffer, Renderer::GraphContext& context, const Renderer::VertexAttributes& vert, const Renderer::DescriptorSetKey& descriptorKey)
	{
		const float PI = 3.14159265359f;
		static const int detail = 30;

		float sweepline = planet->sweepline;

		std::vector<Vertex> vertices;
		vertices.resize(detail + 1);
		
		for(int i = 0; i < detail; i++)
		{
			auto x = sin(sweepline) * cos( 2.0f * PI * i / (float)detail);
			auto y = sin(sweepline) * sin( 2.0f * PI * i / (float)detail);
			auto z = cos(sweepline);

			vertices[i] = Vertex{ {x,y,z}, {1,1,1}};
		}

		vertices[detail] = vertices.front();
		
		if(sweeplineBuffer->GetSize() < sizeof(Vertex) * vertices.size() ) sweeplineBuffer = Memory::Buffer::Resize(alloc, sweeplineBuffer, sizeof(Vertex) * vertices.size() * 2);
		sweeplineBuffer->Load(static_cast<void*>(vertices.data()), sizeof(Vertex) * vertices.size());

		context.GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetDefaultRenderpass(), context.GetSwapchainExtent(), vert, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, descriptorKey.program);

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &sweeplineBuffer->GetResourceHandle(), offsets);
		vkCmdDraw(buffer, vertices.size(), 1, 0, 0);
	}

	void PlanetRenderer::DrawSites(VkCommandBuffer buffer, GraphContext& context, const VertexAttributes& vert, const DescriptorSetKey& descriptorKey)
	{
		if(planet->planet->cells.empty()) return;

		auto planet = this->planet->planet;

		std::vector<Vertex> vertices;

		for(int i = 0; i < planet->cells.size(); i++)
			vertices.emplace_back(Vertex{ planet->cells[i].point, glm::vec3{ 0.75, 0, 0 }});


		if(sitesBuffer->GetSize() < sizeof(Vertex) * vertices.size() ) sitesBuffer = Memory::Buffer::Resize(alloc, sitesBuffer, sizeof(Vertex) * vertices.size() * 2);
		sitesBuffer->Load(static_cast<void*>(vertices.data()), sizeof(Vertex) * vertices.size());

		context.GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetDefaultRenderpass(), context.GetSwapchainExtent(), vert, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, descriptorKey.program);
		
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &sitesBuffer->GetResourceHandle(), offsets);
		vkCmdDraw(buffer, vertices.size(), 1, 0, 0);
	}

	void PlanetRenderer::DrawVoronoiEdges(VkCommandBuffer buffer, GraphContext& context, const VertexAttributes& vert, const DescriptorSetKey& descriptorKey)
	{
		auto planet = this->planet->planet;

		if(planet->halfEdges.empty()) return;

		if(planet->halfEdges.size() != voronoiEdgeCache)
		{
			std::vector<Vertex> vertices;

			for(int j = 0; j < planet->halfEdges.size(); j++)
			{
				auto& edge = planet->halfEdges[j];		
				if(edge.endIndex != ~0u)
				{
					vertices.emplace_back(Vertex{ planet->edgeVertices[edge.beginIndex], glm::vec3{ 0, 1,0 }});
					vertices.emplace_back(Vertex{ planet->edgeVertices[edge.endIndex], glm::vec3{ 0, 1,0 }});
				}
			}

			if(voronoiEdges->GetSize() < sizeof(Vertex) * vertices.size() ) voronoiEdges = Memory::Buffer::Resize(alloc, voronoiEdges, sizeof(Vertex) * vertices.size() * 2);
			voronoiEdges->Load(static_cast<void*>(vertices.data()), sizeof(Vertex) * vertices.size());

			voronoiEdgeCache = planet->halfEdges.size();
		}

		if(voronoiEdgeCache == 0) return;

		context.GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetDefaultRenderpass(), context.GetSwapchainExtent(), vert, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, descriptorKey.program);

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &voronoiEdges->GetResourceHandle(), offsets);
		vkCmdDraw(buffer, voronoiEdgeCache * 2, 1, 0, 0);
	}

	void PlanetRenderer::DrawDelanuayEdges(VkCommandBuffer buffer, Renderer::GraphContext& context, const Renderer::VertexAttributes& vert, const Renderer::DescriptorSetKey& descriptorKey)
	{
		auto planet = this->planet->planet;

		if(planet->delanuayEdges.empty()) return;

		if(planet->delanuayEdges.size() != delanuayEdgeCache)
		{
			std::vector<Vertex> vertices;

			for(int j = 0; j < planet->delanuayEdges.size(); j++)
			{
				auto& edge = planet->delanuayEdges[j];		
				if(edge.endIndex != ~0u)
				{
					vertices.emplace_back(Vertex{ planet->delanuayCells[edge.beginIndex], planet->delanuayCells[edge.beginIndex]});
					vertices.emplace_back(Vertex{ planet->delanuayCells[edge.endIndex], planet->delanuayCells[edge.beginIndex]});
				}
			}

			if(delauneyEdges->GetSize() < sizeof(Vertex) * vertices.size() ) delauneyEdges = Memory::Buffer::Resize(alloc, delauneyEdges, sizeof(Vertex) * vertices.size() * 2);
			delauneyEdges->Load(static_cast<void*>(vertices.data()), sizeof(Vertex) * vertices.size());

			delanuayEdgeCache = planet->delanuayEdges.size();
		}
		
		if(delanuayEdgeCache == 0) return;
		
		context.GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context.GetDefaultRenderpass(), context.GetSwapchainExtent(), vert, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, descriptorKey.program);

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &delauneyEdges->GetResourceHandle(), offsets);
		vkCmdDraw(buffer, delanuayEdgeCache * 2, 1, 0, 0);
	}
}
