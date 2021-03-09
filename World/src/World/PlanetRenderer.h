#pragma once
#include "Generation/PlanetGenerator.h"
#include "volk/volk.h"

namespace Renderer {
	class Core;
	struct DescriptorSetKey;
	struct VertexAttributes;
	namespace RenderGraph
	{
		struct GraphContext;
	}

	namespace Memory {
		class Allocator;
		class Buffer;
	}
}

namespace World
{
	class Planet;


class PlanetRenderer
{
public:
	
	explicit PlanetRenderer(Generation::PlanetGenerator* planet, Renderer::Memory::Allocator* alloc);
	~PlanetRenderer();

	void SetFrameState(Renderer::Core* core, VkCommandBuffer buffer, Renderer::RenderGraph::GraphContext* context, Renderer::VertexAttributes* vert, Renderer::DescriptorSetKey* descriptorKey);
	void DrawBeachline();
	void DrawSweepline();
	void DrawSites();
	void DrawVoronoiEdges();
	void DrawDelanuayEdges();
	void DrawVoronoiFaces();
	void DrawDelanuayFaces();
	
	void Reset(Generation::PlanetGenerator* newGenerator)
	{
		this->generator = newGenerator;

		sitesCache = -1;
		voronoiEdgeCache = -1;
		voronoiEdgeVertices = -1;
		delanuayEdgeCache = -1;
		delanuayEdgeVertices = -1;
		facesCache = -1;
		delanuayFacesVertices = -1;
		delanuayFacesIndices = -1;
	}

private:
	Generation::PlanetGenerator* generator;
	Renderer::Memory::Allocator* alloc;

	// Not necessarily correct, because vertices may not change but their completion status may.
	// If I wanted to do this properly I would look into using index buffers etc.
	int sitesCache = -1;
	int voronoiEdgeCache = -1;
	int voronoiEdgeVertices = -1, voronoiEdgeIndexCount = -1;
	int delanuayEdgeCache = -1;
	int delanuayEdgeVertices = -1;
	int delanuayFacesVertices = -1, delanuayFacesIndices = -1;
	int facesCache = -1;
	
	Renderer::Memory::Buffer* beachlineBuffer = nullptr;
	Renderer::Memory::Buffer* sweeplineBuffer = nullptr;
	Renderer::Memory::Buffer* sitesBuffer = nullptr;
	Renderer::Memory::Buffer* voronoiEdges = nullptr;
	Renderer::Memory::Buffer* delauneyEdges = nullptr;
	Renderer::Memory::Buffer* delanuayFaces = nullptr;
	Renderer::Memory::Buffer* voronoiFaces = nullptr;

	Renderer::Core* core;
	VkCommandBuffer buffer;
	Renderer::RenderGraph::GraphContext* context;
	Renderer::VertexAttributes* vert;
	Renderer::DescriptorSetKey* descriptorKey;
};

}
