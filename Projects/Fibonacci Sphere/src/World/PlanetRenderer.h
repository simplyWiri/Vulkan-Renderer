#pragma once
#include <vulkan_core.h>

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

	void DrawCells(VkCommandBuffer buffer);

	void DrawGreatCircle();

private:
	Planet* planet;
	Renderer::Memory::Buffer* vertexBuffer;
	
};

}