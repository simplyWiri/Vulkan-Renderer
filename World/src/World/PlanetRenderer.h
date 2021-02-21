#pragma once
#include <vulkan_core.h>

#include "Generation/PlanetGenerator.h"

namespace Renderer {
	struct DescriptorSetKey;
	struct VertexAttributes;
	struct GraphContext;
}

namespace Renderer {
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

	void DrawBeachline(VkCommandBuffer buffer, Renderer::GraphContext& context, const Renderer::VertexAttributes& vert, const Renderer::DescriptorSetKey& descriptorKey);
	void DrawSweepline(VkCommandBuffer buffer, Renderer::GraphContext& context, const Renderer::VertexAttributes& vert, const Renderer::DescriptorSetKey& descriptorKey);
	void DrawSites(VkCommandBuffer buffer, Renderer::GraphContext& context, const Renderer::VertexAttributes& vert, const Renderer::DescriptorSetKey& descriptorKey);
	void DrawVoronoiEdges(VkCommandBuffer buffer, Renderer::GraphContext& context, const Renderer::VertexAttributes& vert, const Renderer::DescriptorSetKey& descriptorKey);
	void DrawDelanuayEdges(VkCommandBuffer buffer, Renderer::GraphContext& context, const Renderer::VertexAttributes& vert, const Renderer::DescriptorSetKey& descriptorKey);

private:
	Generation::PlanetGenerator* planet;
	Renderer::Memory::Allocator* alloc;

	int edges = 0;

	// Not necessarily correct, because vertices may not change but their completion status may.
	// If I wanted to do this properly I would look into using index buffers etc.
	int sitesCache = -1;
	int voronoiEdgeCache = -1;
	int voronoiEdgeVertices = -1;
	int delanuayEdgeCache = -1;
	int delanuayEdgeVertices = -1;
	
	Renderer::Memory::Buffer* beachlineBuffer = nullptr;
	Renderer::Memory::Buffer* sweeplineBuffer = nullptr;
	Renderer::Memory::Buffer* sitesBuffer = nullptr;
	Renderer::Memory::Buffer* voronoiEdges = nullptr;
	Renderer::Memory::Buffer* delauneyEdges = nullptr;

};

}
