#pragma once
#include <math.h>
#include <cmath>
#include <vector>
#include "Renderer/Resources/Buffer.h"
#include "Renderer/Vulkan/Core.h"

using namespace Renderer;

struct Point
{
	float x;
	float y;
};

class Sphere
{
public:
	const float PI = 3.14159265359f;
	const float GOLDEN_RATIO = 1.61803398875f;
	const float GOLDEN_ANGLE = GOLDEN_RATIO * 2.0f * PI;

	Memory::Buffer* buf;
	Memory::Buffer* ibuf;

	Memory::Buffer* sweeplineBuf;
	Memory::Buffer* sweeplineIBuf;
	Memory::Allocator* alloc;
	int numPoints;
	float curY = 0;
	bool fibo;

	Point fibonacci(int i)
	{
		auto longitude = GOLDEN_ANGLE * i;
		longitude /= 2 * PI;
		longitude -= floor(longitude);
		longitude *= 2 * PI;

		if (longitude > PI) longitude -= 2 * PI;

		float latitude = asin(-1 + (2 * i / (float)numPoints));

		return Point{ latitude, longitude };
	}

	Point cylindricalDistribution(int i)
	{
		if (i == 0) return Point{ 0,0 };
		if (i == numPoints - 1) return Point{ 1,0 };

		auto longit = (i + 3.5f) / (numPoints + 6);
		auto latit = i / GOLDEN_RATIO;

		return Point{ longit, latit };
	}

	Sphere(Memory::Allocator* alloc, int numPoints)
	{
		this->numPoints = numPoints;
		this->alloc = alloc;
		this->buf = alloc->AllocateBuffer(sizeof(Vertex) * numPoints, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		this->ibuf = alloc->AllocateBuffer(sizeof(uint16_t) * numPoints, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		this->sweeplineBuf = alloc->AllocateBuffer(sizeof(Vertex) * 4, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		this->sweeplineIBuf = alloc->AllocateBuffer(sizeof(uint16_t) * 6, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		this->fibo = false;
		this->curY = 0;

		std::vector<Vertex> originPoints = std::vector<Vertex>(4);
		std::vector<uint16_t> originIndices = std::vector<uint16_t>(6);
		originPoints[0] = { { 0, 0, 0 }, { 1, 0, 0 }};
		originPoints[1] = { { 0, -1, 0}, { 0, 1, 0 }};
		originIndices[0] = 0;
		originIndices[1] = 1;

		originPoints[2] = { { 1, 0, 0}, { 0, 0, 1 }};
		originIndices[2] = 0;
		originIndices[3] = 2;

		originPoints[3] = { { 0, 0, 1 }, { 1, 1, 1}};
		originIndices[4] = 0;
		originIndices[5] = 3;

		sweeplineBuf->Load((void*)originPoints.data(), sizeof(Vertex) * originPoints.size(), 0);
		sweeplineIBuf->Load((void*)originIndices.data(), sizeof(uint16_t) * originIndices.size(), 0);

		
		ReCalculate(numPoints);
	}

	void ReCalculate(int numPoints)
	{
		this->numPoints = numPoints;
		curY = curY + ((float)numPoints / 10000.0f);
		if(curY >= 1) curY -= 1;

		std::vector<Vertex> verts = std::vector<Vertex>(numPoints);
		std::vector<uint16_t> indexes = std::vector<uint16_t>(numPoints);

		for (int i = 0; i < numPoints-1; i++)
		{
			if (fibo)
			{
				Point p = fibonacci(i);

				float x = cos(p.x) * cos(p.y);
				float y = cos(p.x) * sin(p.y);
				float z = sin(p.x);

				verts[i] = { { x, y, z  }, { 0.5f, y, z } };

				indexes[i] = i;
			}
			else
			{
				Point p = cylindricalDistribution(i);

				// x,y -> theta, phi
				auto theta = acos(2 * p.x - 1) - PI / 2;
				auto phi = 2 * PI * p.y;

				// theta, phi -> x,y,z;
				auto x = cos(theta) * cos(phi);
				auto y = cos(theta) * sin(phi);
				auto z = sin(theta);

				verts[i] = { { x, y, z  }, { 0.5f, y, z } };

				indexes[i] = i;
			}
		}

		// Priority Queue of 'Events'
		// Binary Tree for the Beach structure

		// foreach entry in the queue
		//		pop the first entry from the queue
		//		update the beach-line to account for the entry
		//		move the 'shoreline' to the location of the next event in the queue
		//		if there are no events, break.

		// distance between two points (p, q) = acos ( p * q )
		// acos ( p.x * q.x + p.y * q.y + p.z * q.z )

		if(buf->GetSize() < sizeof(Vertex) * numPoints)
		{
			auto usage = buf->GetUsageFlags();
			auto flags = buf->GetMemoryFlags();
			delete buf;
			buf = alloc->AllocateBuffer(sizeof(Vertex) * numPoints, usage, flags);
		}
		if(ibuf->GetSize() < sizeof(uint16_t) * numPoints)
		{
			auto usage = ibuf->GetUsageFlags();
			auto flags = ibuf->GetMemoryFlags();
			delete ibuf;
			ibuf = alloc->AllocateBuffer(sizeof(uint16_t) * numPoints, usage, flags);
		}
		
		buf->Load((void*)verts.data(), sizeof(Vertex) * numPoints);
		ibuf->Load((void*)indexes.data(), sizeof(uint16_t) * numPoints);
	}

	void SetFibo(bool value)
	{
		fibo = value;	
	}

	void Draw(VkCommandBuffer& buffer)
	{
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &buf->GetResourceHandle(), offsets);
		vkCmdBindIndexBuffer(buffer, ibuf->GetResourceHandle(), 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(buffer, numPoints, 1, 0, 0, 0);
	}

	void DrawSweepline(VkCommandBuffer& buffer)
	{
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(buffer, 0, 1, &sweeplineBuf->GetResourceHandle(), offsets);
		vkCmdBindIndexBuffer(buffer, sweeplineIBuf->GetResourceHandle(), 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(buffer, 6, 1, 0, 0, 0);
	}

	void Cleanup()
	{
		delete buf;
		delete ibuf;
	}
};