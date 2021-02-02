#include "PlanetRenderer.h"

#include <vulkan_core.h>
#include "Planet.h"
#include "Renderer/Memory/Allocator.h"
#include "Renderer/Memory/Buffer.h"
#include "Renderer/Resources/Vertex.h"

namespace World
{
	PlanetRenderer::PlanetRenderer(Planet* planet, Renderer::Memory::Allocator* alloc)
		: planet(planet)
	{
		this->vertexBuffer = alloc->AllocateBuffer(sizeof(Renderer::Vertex) * 5000, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		std::vector<Renderer::Vertex> vertices(planet->cells.size());

		for(int i = 0; i < planet->cells.size(); i++)
			vertices[i] = ( Renderer::Vertex{ planet->cells[i].point, glm::vec3{ planet->cells[i].point.x * 0.8f, planet->cells[i].point.y, planet->cells[i].point.z }});

		vertexBuffer->Load(static_cast<void*>(vertices.data()), sizeof(Renderer::Vertex) * 5000);
	}

	PlanetRenderer::~PlanetRenderer()
	{
		delete vertexBuffer;
	}

	void PlanetRenderer::DrawCells(VkCommandBuffer buffer)
	{
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &vertexBuffer->GetResourceHandle(), offsets);
		vkCmdDraw(buffer, planet->cells.size(), 1, 0, 0);
	}
}
