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
		this->vertexBuffer = alloc->AllocateBuffer(sizeof(Renderer::Vertex) * 500, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		this->planetVertexBuffer = alloc->AllocateBuffer(sizeof(Renderer::Vertex) * 2500, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		std::vector<Renderer::Vertex> vertices(planet->cells.size());

		for(int i = 0; i < planet->cells.size(); i++)
			vertices[i] = ( Renderer::Vertex{ planet->cells[i].point, glm::vec3{ 0.5f }});

		vertexBuffer->Load(static_cast<void*>(vertices.data()), sizeof(Renderer::Vertex) * 500);
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

	void PlanetRenderer::DrawVertices(VkCommandBuffer buffer)
	{
		if(!planet->edgeVertices.empty())
		{
			LoadVertices();
			
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(buffer, 0, 1, &planetVertexBuffer->GetResourceHandle(), offsets);
			vkCmdDraw(buffer, planet->edgeVertices.size(), 1, 0, 0);
		}
	}
	void PlanetRenderer::LoadVertices()
	{
		std::vector<Renderer::Vertex> vertices(planet->edgeVertices.size());

		for(int i = 0; i < planet->edgeVertices.size(); i++)
			vertices[i] = ( Renderer::Vertex{ planet->edgeVertices[i], glm::vec3{ 1, 0,0 }});

		planetVertexBuffer->Load(static_cast<void*>(vertices.data()), sizeof(Renderer::Vertex) * planet->edgeVertices.size());
	}
}
