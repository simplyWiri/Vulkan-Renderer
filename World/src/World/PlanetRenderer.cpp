#include "PlanetRenderer.h"

#include "Planet.h"
#include "Renderer/Core.h"
#include "Renderer/Memory/Allocator.h"
#include "Renderer/Memory/Buffer.h"
#include "Renderer/RenderGraph/GraphContext.h"
#include "Renderer/VulkanObjects/DescriptorSet.h"
#include "Renderer/VulkanObjects/Pipeline.h"

namespace World
{
	using namespace Renderer;

	// slightly cursed but functional
	int NextPow2(int n)
	{
		if (n <= 1) return n;
		double d = n - 1;
		return 1 << ((((int*)&d)[1] >> 20) - 1022);
	}

	PlanetRenderer::PlanetRenderer(Generation::PlanetGenerator* planet, Memory::Allocator* alloc) : generator(planet), alloc(alloc)
	{
		auto usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		auto memoryType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		beachlineBuffer = alloc->AllocateBuffer(sizeof(Vertex), usage, memoryType);
		sweeplineBuffer = alloc->AllocateBuffer(sizeof(Vertex), usage, memoryType);
		sitesBuffer = alloc->AllocateBuffer(sizeof(Vertex), usage, memoryType);
		
		voronoi.edgeBuffer = alloc->AllocateBuffer(sizeof(Vertex), usage, memoryType);
		voronoi.faceBuffer = alloc->AllocateBuffer(sizeof(Vertex), usage, memoryType);

		delanuay.edgeBuffer = alloc->AllocateBuffer(sizeof(Vertex), usage, memoryType);
		delanuay.faceBuffer = alloc->AllocateBuffer(sizeof(Vertex), usage, memoryType);
	}

	PlanetRenderer::~PlanetRenderer()
	{
		delete beachlineBuffer;
		delete sweeplineBuffer;
		delete sitesBuffer;

		voronoi.Cleanup();
		delanuay.Cleanup();;
	}

	void PlanetRenderer::SetFrameState(Core* core, VkCommandBuffer buffer, RenderGraph::GraphContext* context, VertexAttributes* vert, DescriptorSetKey* descriptorKey)
	{
		this->core = core;
		this->buffer = buffer;
		this->context = context;
		this->vert = vert;
		this->descriptorKey = descriptorKey;
	}

	void PlanetRenderer::DrawBeachline()
	{
		if (generator->beach.Count() < 3) return;

		core->GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context->GetRenderpass(), context->GetExtent(), *vert, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, descriptorKey->program);

		auto* current = generator->beach.First();

		std::vector<Vertex> vertices;
		do
		{
			vertices.emplace_back(Vertex{ current->element.site.position, { .5, .5, 0 } });
			current = current->Successor();
		}
		while (current != nullptr);

		vertices.push_back(vertices.front());

		if (beachlineBuffer->GetSize() < sizeof(Vertex) * vertices.size()) beachlineBuffer = Memory::Buffer::Resize(alloc, beachlineBuffer, sizeof(Vertex) * vertices.size() * 2);
		beachlineBuffer->Load(static_cast<void*>(vertices.data()), sizeof(Vertex) * vertices.size());

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &beachlineBuffer->GetResourceHandle(), offsets);
		vkCmdDraw(buffer, vertices.size(), 1, 0, 0);
	}

	void PlanetRenderer::DrawSweepline()
	{
		const float PI = 3.14159265359f;
		static const int detail = 30;

		float sweepline = generator->sweepline;

		std::vector<Vertex> vertices;
		vertices.resize(detail + 1);

		for (int i = 0; i < detail; i++)
		{
			auto x = sin(sweepline) * cos(2.0f * PI * i / (float)detail);
			auto y = sin(sweepline) * sin(2.0f * PI * i / (float)detail);
			auto z = cos(sweepline);

			vertices[i] = Vertex{ { x, y, z }, { 1, 1, 1 } };
		}

		vertices[detail] = vertices.front();

		if (sweeplineBuffer->GetSize() < sizeof(Vertex) * vertices.size()) sweeplineBuffer = Memory::Buffer::Resize(alloc, sweeplineBuffer, sizeof(Vertex) * vertices.size() * 2);
		sweeplineBuffer->Load(static_cast<void*>(vertices.data()), sizeof(Vertex) * vertices.size());

		core->GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context->GetRenderpass(), context->GetExtent(), *vert, DepthSettings::Disabled(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, descriptorKey->program);

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &sweeplineBuffer->GetResourceHandle(), offsets);
		vkCmdDraw(buffer, vertices.size(), 1, 0, 0);
	}

	void PlanetRenderer::DrawSites()
	{
		if (generator->planet->cells.empty()) return;

		auto* planet = this->generator->planet;

		if (sitesCache != planet->cells.size())
		{
			std::vector<Vertex> vertices(planet->cells.size() - sitesCache);

			for (int i = sitesCache; i < planet->cells.size(); i++) vertices[i - sitesCache] = Vertex{ planet->cells[i].GetCenter(), glm::vec3{ 1 } };

			if (sitesBuffer->GetSize() < sizeof(Vertex) * planet->cells.capacity()) sitesBuffer = Memory::Buffer::Resize(alloc, sitesBuffer, sizeof(Vertex) * planet->cells.capacity());
			sitesBuffer->Load(static_cast<void*>(vertices.data()), sizeof(Vertex) * vertices.size(), sizeof(Vertex) * sitesCache);

			sitesCache += vertices.size();
		}
		core->GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context->GetRenderpass(), context->GetExtent(), *vert, DepthSettings::DepthTest(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, descriptorKey->program);

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &sitesBuffer->GetResourceHandle(), offsets);
		vkCmdDraw(buffer, sitesCache, 1, 0, 0);
	}

	void PlanetRenderer::DrawVoronoiEdges()
	{
		static const glm::vec3 color = glm::vec3(0.3f);
		
		auto planet = this->generator->planet;

		if (planet->halfEdges.empty()) return;

		if (planet->halfEdges.size() != voronoi.edgeVertexCount / 2)
		{
			auto& vertices = voronoi.edgeVertices;

			if(vertices.size() != planet->halfEdges.size() * 2) vertices.resize(planet->halfEdges.size() * 2, Vertex{ glm::vec3{0.0f}, glm::vec3{0.0f} });			

			// Go through edges we have previously looked at, but weren't finished then, remove them if they have been finished + update vertices in the array
			voronoi.unfinishedEdges.erase(std::remove_if(voronoi.unfinishedEdges.begin(), voronoi.unfinishedEdges.end(), [&](const auto& idx)
			{
				auto& edge = planet->halfEdges[idx];
				if(edge.endIndex == ~0u) return false;
					
				vertices[idx * 2] = Vertex{planet->edgeVertices[edge.beginIndex] * 1.005f, color};
				vertices[(idx * 2) + 1] = Vertex{planet->edgeVertices[edge.endIndex]  * 1.005f, color};

				return true;
			}), voronoi.unfinishedEdges.end());

			// Go through the edges we have not looked at yet
			for (int j = voronoi.currentUpatedEdgeIndex; j < planet->halfEdges.size(); j++)
			{
				auto& edge = planet->halfEdges[j];
				if (edge.endIndex != ~0u)
				{					
					vertices[j * 2] = Vertex{planet->edgeVertices[edge.beginIndex] * 1.005f, color };
					vertices[(j * 2) + 1] = Vertex{planet->edgeVertices[edge.endIndex] * 1.005f, color };
				} else
				{
					voronoi.unfinishedEdges.emplace_back(j);
				}
			}
			
			auto size = sizeof(Vertex) * vertices.size();

			if (voronoi.edgeBuffer->GetSize() < size) voronoi.edgeBuffer = Memory::Buffer::Resize(alloc, voronoi.edgeBuffer, size);

			voronoi.edgeBuffer->Load(static_cast<void*>(vertices.data()), sizeof(Vertex) * vertices.size());

			voronoi.edgeVertexCount = vertices.size() - voronoi.unfinishedEdges.size() * 2;
			voronoi.currentUpatedEdgeIndex = planet->halfEdges.size();
		}

		if (voronoi.edgeVertexCount == 0) return;

		core->GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context->GetRenderpass(), context->GetExtent(), *vert, DepthSettings::DepthTest(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, descriptorKey->program);

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &voronoi.edgeBuffer->GetResourceHandle(), offsets);
		vkCmdDraw(buffer, voronoi.edgeVertices.size(), 1, 0, 0);
	}

	void PlanetRenderer::DrawDelanuayEdges()
	{
		auto planet = this->generator->planet;

		if (planet->delanuayEdges.empty()) return;

		if (planet->delanuayEdges.size() != delanuay.edgeVertexCount / 2)
		{
			auto& vertices = delanuay.edgeVertices;

			if(vertices.size() != planet->delanuayEdges.size() * 2) vertices.resize(planet->delanuayEdges.size() * 2, Vertex{ glm::vec3{0.0f}, glm::vec3{0.0f} });			

			// Go through edges we have previously looked at, but weren't finished then, remove them if they have been finished + update vertices in the array
			delanuay.unfinishedEdges.erase(std::remove_if(delanuay.unfinishedEdges.begin(), delanuay.unfinishedEdges.end(), [&](const auto& idx)
			{
				auto& edge = planet->delanuayEdges[idx];
				if(edge.endIndex == ~0u) return false;
					
				vertices[idx * 2] = Vertex{planet->cells[edge.beginIndex].GetCenter() * 1.005f, glm::vec3(.8f * (edge.beginIndex / static_cast<float>(planet->cells.capacity())), .3, .3) };
				vertices[(idx * 2) + 1] = Vertex{planet->cells[edge.endIndex].GetCenter() * 1.005f, glm::vec3(.8f * (edge.endIndex / static_cast<float>(planet->cells.capacity())), .3, .3)};

				return true;
			}), delanuay.unfinishedEdges.end());

			// Go through the edges we have not looked at yet
			for (int j = delanuay.currentUpatedEdgeIndex; j < planet->delanuayEdges.size(); j++)
			{
				auto& edge = planet->delanuayEdges[j];
				if (edge.endIndex != ~0u)
				{
					// Alternate colouring glm::vec3{0.25 } + planet->cells[edge.beginIndex].GetCenter() * 0.5f
					
					// extrude these a little bit to draw them ontop of the faces.
					vertices[j * 2] = Vertex{planet->cells[edge.beginIndex].GetCenter() * 1.005f, glm::vec3(.8f * (edge.beginIndex / static_cast<float>(planet->cells.capacity())), .3, .3) };
					vertices[(j * 2) + 1] = Vertex{planet->cells[edge.endIndex].GetCenter() * 1.005f, glm::vec3(.8f * (edge.endIndex / static_cast<float>(planet->cells.capacity())), .3, .3)};
				} else
				{
					delanuay.unfinishedEdges.emplace_back(j);
				}
			}

			if (delanuay.edgeBuffer->GetSize() < sizeof(Vertex) * vertices.size()) delanuay.edgeBuffer = Memory::Buffer::Resize(alloc, delanuay.edgeBuffer, sizeof(Vertex) * vertices.size() * 2);
			delanuay.edgeBuffer->Load(static_cast<void*>(vertices.data()), sizeof(Vertex) * vertices.size());

			delanuay.edgeVertexCount = vertices.size() - delanuay.unfinishedEdges.size() * 2;
			delanuay.currentUpatedEdgeIndex = planet->delanuayEdges.size();
		}

		if (delanuay.edgeVertexCount == 0) return;

		core->GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context->GetRenderpass(), context->GetExtent(), *vert, DepthSettings::DepthTest(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, descriptorKey->program);

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &delanuay.edgeBuffer->GetResourceHandle(), offsets);
		vkCmdDraw(buffer, delanuay.edgeVertices.size(), 1, 0, 0);
	}

	void PlanetRenderer::DrawVoronoiFaces()
	{
		if (generator->Finished() == false) return;

		if (voronoi.faceVertexCount == - 1)
		{
			std::vector<Vertex> vertices;
			auto& voronoiCells = generator->planet->cells;
			const auto& halfEdges = generator->planet->halfEdges;
			const auto& edgeVertices = generator->planet->edgeVertices;

			for (int i = 0; i < voronoiCells.size(); i++)
			{
				auto& cell = voronoiCells[i];
				const auto color = glm::vec3{ 0.25 } + cell.GetCenter() * 0.5f;

				auto verts = cell.GetFaceVertices(halfEdges, edgeVertices);

				for (int i = 0; i < verts.size(); i += 3)
				{
					auto& c1 = verts[i];
					auto& c2 = verts[i+1];
					auto& c3 = verts[i+2];

					auto centroid = glm::vec3{ (c1.x + c2.x + c3.x) / 3.0f, (c1.y + c2.y + c3.y) / 3.0f, (c1.z + c2.z + c3.z) / 3.0f };
					auto col = glm::vec3{ 0.25 } + centroid * 0.5f;

					vertices.emplace_back(Vertex{ c1, col });
					vertices.emplace_back(Vertex{ c2, col });
					vertices.emplace_back(Vertex{ c3, col });
				}
			}

			voronoi.faceVertexCount = vertices.size();

			if (voronoi.faceBuffer->GetSize() < sizeof(Vertex) * vertices.size()) voronoi.faceBuffer = Memory::Buffer::Resize(alloc, voronoi.faceBuffer, sizeof(Vertex) * vertices.size());
			voronoi.faceBuffer->Load(static_cast<void*>(vertices.data()), sizeof(Vertex) * vertices.size());
		}

		core->GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context->GetRenderpass(), context->GetExtent(), *vert, DepthSettings::DepthTest(), { BlendSettings::Opaque() }, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			descriptorKey->program);

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &voronoi.faceBuffer->GetResourceHandle(), offsets);
		vkCmdDraw(buffer, voronoi.faceVertexCount, 1, 0, 0);
	}

	void PlanetRenderer::DrawDelanuayFaces()
	{
		if (generator->Finished() == false) return;

		auto* planet = generator->planet;

		if (delanuay.faceVertexCount == - 1)
		{
			std::vector<Vertex> vertices;

			auto& voronoiCells = generator->planet->cells;
			auto& delEdges = generator->planet->delanuayEdges;

			for (int i = 0; i < voronoiCells.size(); i++)
			{
				auto cellEdges = voronoiCells[i].GetSortedDelEdges(planet->delanuayEdges, planet->cells);

				const auto color = glm::vec3{ 0.25 } + planet->cells[i].GetCenter() * 0.5f;

				for (int j = 0; j < cellEdges.size(); j++)
				{
					auto& resolvedEdge = delEdges[cellEdges[j]];
					auto& resolvedFollowingEdge = delEdges[cellEdges[(j + 1) % cellEdges.size()]];

					auto idx = resolvedEdge.beginIndex == i ? resolvedEdge.endIndex : resolvedEdge.beginIndex;
					auto idx2 = resolvedFollowingEdge.beginIndex == i ? resolvedFollowingEdge.endIndex : resolvedFollowingEdge.beginIndex;
					auto idx3 = i;

					if (idx != idx2 && idx != idx3 && idx2 != idx3)
					{
						auto c1 = planet->cells[idx].GetCenter();
						auto c2 = planet->cells[idx2].GetCenter();
						auto c3 = planet->cells[idx3].GetCenter();

						auto centroid = glm::vec3{ (c1.x + c2.x + c3.x) / 3.0f, (c1.y + c2.y + c3.y) / 3.0f, (c1.z + c2.z + c3.z) / 3.0f };
						auto col = glm::vec3{ 0.25 } + centroid * 0.5f;

						vertices.emplace_back(Vertex{ c1, col });
						vertices.emplace_back(Vertex{ c2, col });
						vertices.emplace_back(Vertex{ c3, col });
					}
				}
			}

			delanuay.faceVertexCount = vertices.size();

			auto size = sizeof(Vertex) * vertices.size();

			if (delanuay.faceBuffer->GetSize() < size) delanuay.faceBuffer = Memory::Buffer::Resize(alloc, delanuay.faceBuffer, size);
			delanuay.faceBuffer->Load(static_cast<void*>(vertices.data()), sizeof(Vertex) * vertices.size());
		}

		core->GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context->GetRenderpass(), context->GetExtent(), *vert, DepthSettings::DepthTest(), { BlendSettings::AlphaBlend() }, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			descriptorKey->program);

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &delanuay.faceBuffer->GetResourceHandle(), offsets);
		vkCmdDraw(buffer, delanuay.faceVertexCount, 1, 0, 0);
	}

	void PlanetRenderer::Voronoi::Cleanup()
	{
		edgeVertexCount = -1;
		faceVertexCount = -1;
		currentUpatedEdgeIndex = 0;
	}
	
	void PlanetRenderer::Delanuay::Cleanup()
	{
		edgeVertexCount = -1;
		faceVertexCount = -1;
		currentUpatedEdgeIndex = 0;

		if(edgeVertices.size() != 0) edgeVertices.clear();
	}
}
