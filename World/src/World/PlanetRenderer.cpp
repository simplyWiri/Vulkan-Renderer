#include "PlanetRenderer.h"

#include "Planet.h"
#include "Renderer/Core.h"
#include "Renderer/Memory/Allocator.h"
#include "Renderer/Memory/Buffer.h"
#include "Renderer/RenderGraph/GraphContext.h"
#include "Renderer/Resources/Vertex.h"
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


		delanuayFaces = alloc->AllocateBuffer(sizeof(Vertex), usage, memoryType);
		beachlineBuffer = alloc->AllocateBuffer(sizeof(Vertex), usage, memoryType);
		sweeplineBuffer = alloc->AllocateBuffer(sizeof(Vertex), usage, memoryType);
		sitesBuffer = alloc->AllocateBuffer(sizeof(Vertex), usage, memoryType);
		voronoiEdges = alloc->AllocateBuffer(sizeof(Vertex), usage, memoryType);
		delauneyEdges = alloc->AllocateBuffer(sizeof(Vertex), usage, memoryType);
		voronoiFaces = alloc->AllocateBuffer(sizeof(Vertex), usage, memoryType);
	}

	PlanetRenderer::~PlanetRenderer()
	{
		delete beachlineBuffer;
		delete sweeplineBuffer;
		delete sitesBuffer;
		delete voronoiEdges;
		delete delauneyEdges;
		delete voronoiFaces;
		delete delanuayFaces;
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
			std::vector<Vertex> vertices(planet->cells.size());

			for (int i = 0; i < planet->cells.size(); i++) vertices[i] = Vertex{ planet->cells[i].GetCenter(), glm::vec3{ 1 } };


			if (sitesBuffer->GetSize() < sizeof(Vertex) * vertices.size()) sitesBuffer = Memory::Buffer::Resize(alloc, sitesBuffer, sizeof(Vertex) * vertices.size() * 2);
			sitesBuffer->Load(static_cast<void*>(vertices.data()), sizeof(Vertex) * vertices.size());

			sitesCache = vertices.size();
		}
		core->GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context->GetRenderpass(), context->GetExtent(), *vert, DepthSettings::DepthTest(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, descriptorKey->program);

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &sitesBuffer->GetResourceHandle(), offsets);
		vkCmdDraw(buffer, sitesCache, 1, 0, 0);
	}

	void PlanetRenderer::DrawVoronoiEdges()
	{
		auto planet = this->generator->planet;

		if (planet->halfEdges.empty()) return;

		if (planet->halfEdges.size() != voronoiEdgeCache)
		{
			std::vector<Vertex> vertices(planet->edgeVertices.size());
			std::vector<uint16_t> indices;

			for (int i = 0; i < planet->edgeVertices.size(); i++) vertices[i] = Vertex{ planet->edgeVertices[i] * 1.005f, glm::vec3(.3f) };

			for (int j = 0; j < planet->halfEdges.size(); j++)
			{
				auto& edge = planet->halfEdges[j];
				if (edge.endIndex != ~0u)
				{
					indices.emplace_back(edge.beginIndex);
					indices.emplace_back(edge.endIndex);
				}
			}

			auto requiredSize = (sizeof(Vertex) * vertices.size()) + (sizeof(uint16_t) * indices.size());

			if (voronoiEdges->GetSize() < requiredSize) voronoiEdges = Memory::Buffer::Resize(alloc, voronoiEdges, NextPow2(requiredSize));

			voronoiEdges->Load(static_cast<void*>(vertices.data()), sizeof(Vertex) * vertices.size());
			voronoiEdges->Load(static_cast<void*>(indices.data()), sizeof(uint16_t) * indices.size(), sizeof(Vertex) * vertices.size());

			voronoiEdgeCache = planet->halfEdges.size();
			voronoiEdgeVertices = vertices.size();
			voronoiEdgeIndexCount = indices.size();
		}

		if (voronoiEdgeVertices == 0 || voronoiEdgeIndexCount == 0) return;

		core->GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context->GetRenderpass(), context->GetExtent(), *vert, DepthSettings::DepthTest(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, descriptorKey->program);

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &voronoiEdges->GetResourceHandle(), offsets);
		vkCmdBindIndexBuffer(buffer, voronoiEdges->GetResourceHandle(), sizeof(Vertex) * voronoiEdgeVertices, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(buffer, voronoiEdgeIndexCount, 1, 0, 0, 0);
	}

	void PlanetRenderer::DrawDelanuayEdges()
	{
		auto planet = this->generator->planet;

		if (planet->delanuayEdges.empty()) return;

		if (planet->delanuayEdges.size() != delanuayEdgeCache)
		{
			std::vector<Vertex> vertices;

			for (int j = 0; j < planet->delanuayEdges.size(); j++)
			{
				auto& edge = planet->delanuayEdges[j];
				if (edge.endIndex != ~0u)
				{
					// extrude these a little bit to draw them ontop of the faces.
					vertices.emplace_back(Vertex{
						planet->cells[edge.beginIndex].GetCenter() * 1.005f, glm::vec3(.8f * edge.beginIndex / (float)planet->cells.size(), .3, .3) /*glm::vec3{0.25 } + planet->cells[edge.beginIndex].GetCenter() * 0.5f*/
					});
					vertices.emplace_back(Vertex{
						planet->cells[edge.endIndex].GetCenter() * 1.005f, glm::vec3(.8f * edge.endIndex / (float)planet->cells.size(), .3, .3) /*glm::vec3{0.25 } + planet->cells[edge.endIndex].GetCenter() * 0.5f*/
					});
				}
			}

			if (delauneyEdges->GetSize() < sizeof(Vertex) * vertices.size()) delauneyEdges = Memory::Buffer::Resize(alloc, delauneyEdges, sizeof(Vertex) * vertices.size() * 2);
			delauneyEdges->Load(static_cast<void*>(vertices.data()), sizeof(Vertex) * vertices.size());

			delanuayEdgeCache = planet->delanuayEdges.size();
			delanuayEdgeVertices = vertices.size();
		}

		if (delanuayEdgeVertices == 0) return;

		core->GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context->GetRenderpass(), context->GetExtent(), *vert, DepthSettings::DepthTest(), { BlendSettings::Add() }, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, descriptorKey->program);

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &delauneyEdges->GetResourceHandle(), offsets);
		vkCmdDraw(buffer, delanuayEdgeVertices, 1, 0, 0);
	}

	void PlanetRenderer::DrawVoronoiFaces()
	{
		if (generator->Finished() == false) return;

		if (facesCache == - 1)
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

			facesCache = vertices.size();

			if (voronoiFaces->GetSize() < sizeof(Vertex) * vertices.size()) voronoiFaces = Memory::Buffer::Resize(alloc, voronoiFaces, sizeof(Vertex) * vertices.size());
			voronoiFaces->Load(static_cast<void*>(vertices.data()), sizeof(Vertex) * vertices.size());
		}

		core->GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context->GetRenderpass(), context->GetExtent(), *vert, DepthSettings::DepthTest(), { BlendSettings::Opaque() }, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			descriptorKey->program);

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &voronoiFaces->GetResourceHandle(), offsets);
		vkCmdDraw(buffer, facesCache, 1, 0, 0);
	}

	void PlanetRenderer::DrawDelanuayFaces()
	{
		if (generator->Finished() == false) return;

		auto* planet = generator->planet;

		if (delanuayFacesVertices == - 1)
		{
			std::vector<Vertex> vertices;
			//std::vector<uint16_t> indices;

			//for(int i = 0; i < planet->cells.size(); i++)
			//{				
			//	const auto color = glm::vec3{0.25 } + planet->cells[i].GetCenter() * 0.5f;
			//	const auto oColor = glm::vec3(.8f * i / (float)planet->cells.size(), .3, .3);
			//	vertices[i] = Vertex{planet->cells[i].GetCenter() * .9995f, oColor};
			//}
			//
			auto& voronoiCells = generator->planet->cells;
			auto& delEdges = generator->planet->delanuayEdges;

			//for(int i = 0; i < voronoiCells.size(); i++)
			//{
			//	auto& cellEdges = voronoiCells[i].delEdges;

			//	for(int j = 0; j < cellEdges.size(); j++)
			//	{
			//		const auto& resolvedEdge = delEdges[cellEdges[j]];
			//		const auto& resolvedFollowingEdge = delEdges[cellEdges[(j + 1) % cellEdges.size()]];

			//		auto idx = resolvedEdge.beginIndex == i ? resolvedEdge.endIndex : resolvedEdge.beginIndex;
			//		auto idx2 = resolvedFollowingEdge.beginIndex == i ? resolvedFollowingEdge.endIndex : resolvedFollowingEdge.beginIndex;
			//		auto idx3 = i;

			//		if(idx != idx2 && idx != idx3 && idx2 != idx3)
			//		{
			//			indices.emplace_back(idx);
			//			indices.emplace_back(idx2);
			//			indices.emplace_back(idx3);
			//		}
			//	}
			//}

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


			delanuayFacesVertices = vertices.size();
			//delanuayFacesIndices = indices.size();

			auto size = (sizeof(Vertex) * delanuayFacesVertices)/* + (sizeof(uint16_t) * delanuayFacesIndices)*/;

			if (delanuayFaces->GetSize() < size) delanuayFaces = Memory::Buffer::Resize(alloc, delanuayFaces, size);
			//
			delanuayFaces->Load(static_cast<void*>(vertices.data()), sizeof(Vertex) * vertices.size());
			//delanuayFaces->Load(static_cast<void*>(indices.data()), sizeof(uint16_t) * indices.size(), sizeof(Vertex) * delanuayFacesVertices);
		}

		core->GetGraphicsPipelineCache()->BindGraphicsPipeline(buffer, context->GetRenderpass(), context->GetExtent(), *vert, DepthSettings::DepthTest(), { BlendSettings::AlphaBlend() }, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			descriptorKey->program);

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &delanuayFaces->GetResourceHandle(), offsets);
		//vkCmdBindIndexBuffer(buffer, delanuayFaces->GetResourceHandle(), sizeof(Vertex) * delanuayFacesVertices, VK_INDEX_TYPE_UINT16);
		vkCmdDraw(buffer, delanuayFacesVertices, 1, 0, 0);
	}
}
