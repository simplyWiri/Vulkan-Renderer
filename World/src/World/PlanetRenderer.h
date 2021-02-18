#pragma once
#include <vulkan_core.h>

#include "Generation/PlanetGenerator.h"

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
	
	explicit PlanetRenderer(World::Planet* planet, Renderer::Memory::Allocator* alloc);
	~PlanetRenderer();
	
	void DrawCells(VkCommandBuffer buffer);
	void DrawVertices(VkCommandBuffer buffer);

	void LoadVertices();

private:
	Planet* planet;

	Renderer::Memory::Buffer* planetVertexBuffer;
	Renderer::Memory::Buffer* vertexBuffer;
};

}
