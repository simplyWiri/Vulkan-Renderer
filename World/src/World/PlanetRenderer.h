#pragma once
#include "Generation/PlanetGenerator.h"
#include "Renderer/Resources/Vertex.h"
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

		sitesCache = 0;

		voronoi.Cleanup();
		delanuay.Cleanup();
	}

private:
	Generation::PlanetGenerator* generator;
	Renderer::Memory::Allocator* alloc;


	// We always want this to be updating, no need for caching
	Renderer::Memory::Buffer* beachlineBuffer = nullptr;
	// ditto
	Renderer::Memory::Buffer* sweeplineBuffer = nullptr;

	// Updates here are linear.. we just set a counter for which vertex we have last created a vertex for.
	// this is set once and forget
	int sitesCache = 0;
	Renderer::Memory::Buffer* sitesBuffer = nullptr;

	// We want to cache updates
	struct Voronoi
	{
		// We update this on the fly, and as such want a way to quickly update it without a full o(n) reconstruction
		int currentUpatedEdgeIndex = 0;
		std::vector<Renderer::Vertex> edgeVertices;
		std::vector<uint32_t> unfinishedEdges;
		int edgeVertexCount = -1;
		Renderer::Memory::Buffer* edgeBuffer = nullptr;

		// We only calculate this once when it has finished processing
		int faceVertexCount = -1;
		Renderer::Memory::Buffer* faceBuffer = nullptr;

		void Cleanup();
	} voronoi;

	// Same as voronoi
	struct Delanuay
	{
		int currentUpatedEdgeIndex = 0;
		std::vector<Renderer::Vertex> edgeVertices;
		std::vector<uint32_t> unfinishedEdges;
		int edgeVertexCount = -1;
		Renderer::Memory::Buffer* edgeBuffer = nullptr;

		int faceVertexCount = -1;
		Renderer::Memory::Buffer* faceBuffer = nullptr;

		void Cleanup();
	} delanuay;

	Renderer::Core* core;
	VkCommandBuffer buffer;
	Renderer::RenderGraph::GraphContext* context;
	Renderer::VertexAttributes* vert;
	Renderer::DescriptorSetKey* descriptorKey;
};

}
