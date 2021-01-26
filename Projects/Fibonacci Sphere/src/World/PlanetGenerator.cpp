#include "PlanetGenerator.h"
#include "Renderer/Memory/Allocator.h"
#include "Renderer/Memory/Buffer.h"
#include <algorithm>
#include <Renderer\Resources\Vertex.h>


#include "Renderer/RenderGraph/GraphContext.h"
#include "Renderer/VulkanObjects/DescriptorSet.h"
#include "Renderer/VulkanObjects/Pipeline.h"

PlanetGenerator::PlanetGenerator(Renderer::Memory::Allocator* alloc)
{
	this->buf = alloc->AllocateBuffer(sizeof(Renderer::Vertex) * 5000, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	this->scanlineBuf = alloc->AllocateBuffer(sizeof(Renderer::Vertex) * 360, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	GeneratePoints(5000);
	InitialiseEvents(points);
}

PlanetGenerator::~PlanetGenerator() { delete buf; }

void PlanetGenerator::DrawPlanet(VkCommandBuffer buffer)
{
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(buffer, 0, 1, &buf->GetResourceHandle(), offsets);
	vkCmdDraw(buffer, (int)points.size(), 1, 0, 0);
	
	vkCmdBindVertexBuffers(buffer, 0, 1, &scanlineBuf->GetResourceHandle(), offsets);
	vkCmdDraw(buffer, 360, 1, 0, 0);
}

void PlanetGenerator::Step(float maxDeltaX)
{
	if (!Finished())
	{
		auto nextX = scanline.x + maxDeltaX;

		//LogInfo("Step: {}, Current X: {} - Sites: {} | Circles: {}", steps, scanline.x, siteEventQueue.size(), circleEventQueue.size());

		// if site event queue is not empty, and the site is before any events in the circle queue
		if (!siteEventQueue.empty() && (circleEventQueue.empty() || siteEventQueue.front()->point.theta < circleEventQueue.front()->theta))
		{
			auto siteEvent = siteEventQueue.front();

			if (siteEvent->point.theta <= nextX)
			{
				LogInfo("Handling Site Event");
				scanline.x = siteEvent->point.theta;
				HandleSiteEvent(siteEvent);
				siteEventQueue.erase(siteEventQueue.begin());
			}
			else { scanline.x = nextX; }
		}
		else if (!circleEventQueue.empty())
		{
			auto circleEvent = circleEventQueue.front();

			if(circleEvent->theta <= nextX)
			{
				LogInfo("Handling Circle Event");
				scanline.x = circleEvent->theta;
				HandleCircleEvent(circleEvent);
				circleEventQueue.erase(circleEventQueue.end());
			} else
			{
				scanline.x = nextX;
			}
		}
		else
		{
			scanline.x = nextX;
		}

		std::vector<Renderer::Vertex> verts(360);
		constexpr float degToRad = 180.0f/3.14159265359f;
		
		for(float i = 0; i < 360; i++)
		{

			auto x = cos(scanline.x) * cos(i * degToRad);
			auto y = cos(scanline.x) * sin(i * degToRad);
			auto z = sin(scanline.x);
			verts[i] = {  { x, y, z}, {1, 0, 0} };
		}

		
		scanlineBuf->Load(static_cast<void*>(verts.data()), sizeof(Renderer::Vertex) * 360);
	}

	steps++;
}

void PlanetGenerator::GeneratePoints(int numPoints)
{
	const float PI = 3.14159265359f;
	auto verts = std::vector<Renderer::Vertex>(numPoints);

	for (auto i = 0; i < numPoints; i++)
	{
		auto p = GeneratePoint(i, numPoints);

		// x,y -> theta, phi
		auto theta = acos(2 * p.first - 1) - PI / 2;
		auto phi = 2 * PI * p.second;

		// theta, phi -> x,y,z;
		auto x = cos(theta) * cos(phi);
		auto y = cos(theta) * sin(phi);
		auto z = sin(theta);

		points.emplace_back(glm::vec3(x, y, z));

		verts[i] = { { x, y, z }, { 0.5f, y, z } };
	}

	buf->Load(static_cast<void*>(verts.data()), sizeof(Renderer::Vertex) * numPoints);
}

std::pair<float, float> PlanetGenerator::GeneratePoint(int index, int numPoints)
{
	const float GOLDEN_RATIO = 1.61803398875f;

	if (index == 0) return { 0.0f, 0.0f };
	if (index == numPoints - 1) return { 1.0f, 0.0f };

	auto longit = (index + 3.5f) / (numPoints + 6);
	auto latit = index / GOLDEN_RATIO;

	return { longit, latit };
}

// Sort the points P, and insert into the site event queue
void PlanetGenerator::InitialiseEvents(const std::vector<glm::vec3>& points)
{
	std::vector<std::pair<int, glm::vec3>> sortedPoints(points.size());

	for (const auto& p : points) sortedPoints.emplace_back(std::make_pair((int)sortedPoints.size(), p));

	std::sort(sortedPoints.begin(), sortedPoints.end(), [](const std::pair<int, glm::vec3>& a, const std::pair<int, glm::vec3>& b) { return a.second.z > b.second.z; });

	// Create the priority queue of events (siteEventQueue)
	for (auto i = 0; i < sortedPoints.size(); ++i)
	{
		auto& point = sortedPoints[i];

		Point p(point.second);

		auto cell = new Cell(i, p);
		cells.emplace_back(cell);

		auto it = std::lower_bound(siteEventQueue.begin(), siteEventQueue.end(), cell);
		siteEventQueue.emplace(it, cell);
	}
}

void PlanetGenerator::HandleSiteEvent(Cell* event)
{
	const float PI = 3.14159265359f;
	
	if(beach.Empty())
	{
		// Insert p into the skiplist, set the previous and next pointers to point to itself
		beach.Insert(event->point.phi, new BeachArc(event));
	} else if (beach.Size() == 1)
	{
		// Append p to the skiplist, and reset pointers correspondingly
		beach.Insert(event->point.phi, new BeachArc(event));
	} else
	{
		// Search the skiplist from the reference position for the arc which will intersect with the great circle
		// which goes through `event` and the north pole
		auto arc = beach.GetHead()->right[0];
		while (arc != NULL)
		{
			auto prevArc = arc->left[0];
			auto nextArc = arc->right[0];

			Point previousArcIntersectPoint;
			float phiStart = arc->value->cell->point.phi - PI;
			
			if(prevArc != NULL && arc->value->Intersect(*prevArc->value, scanline.x, previousArcIntersectPoint))
			{
				phiStart = previousArcIntersectPoint.phi;
			}
			

			Point nextArcIntersectPoint;
			float phiEnd = arc->value->cell->point.phi + PI;
			
			if(nextArc != NULL && arc->value->Intersect(*nextArc->value, scanline.x, nextArcIntersectPoint))
			{
				phiEnd = nextArcIntersectPoint.phi;
			}

			if( !( phiStart <= phiEnd && phiStart <= event->point.phi && event->point.phi <= phiEnd ) || ( event->point.phi < phiEnd || event->point.phi > phiStart) ) break;
			
			// assume that the two end points of the arc are (theta, phi) (theta2, phi2)

			// if ( arc.phi_start < arc.phi_end )
			//     event.phi >= arc.phi_start && event.phi <= arc.phi_end
			// else if (arc.phi_start > arc.phi_end
			//     event.phi <= min(arc.phi_start, arc.phi_end) || event.phi >= max(arc.phi_start, arc.phi_end)

			arc = arc->right[0];
		}

		// If either of those conditions are true, we have found the correct arc.

		// if ( arc.circleEvent != null ) // false alarm, remove the event from the circleEventQueue

		// Duplicate arc a and insert the new arc for `event` between them in the skiplist

		// check (p2, pj, pi) and (pi, pj, p3) for valid circle events	
	}
}
void PlanetGenerator::HandleCircleEvent(CircleEvent* event)
{
	// Remove the site `j` which represents the disappearing arc from the skiplist and remove the circle event from the circleQueue
	beach.Erase(event->j->cell->point.phi);
	std::remove(circleEventQueue.begin(), circleEventQueue.end(), event);

	// Add thing to vertex list `event->center`

	// Remove all circle events associated with the triples (1, i, j) and (j, k, 2)

	// Check the two new triples of consecutive arcs (1, i, k) and (i, k, 2)
	// CheckForValidCircleEvent(1, i, k);
	// CheckForValidCircleEvent(i, k, 2);
}

void PlanetGenerator::CheckForValidCircleEvent(Cell* i, Cell* j, Cell* k, float sweeplineX)
{
	auto pij = i->point.position - j->point.position;
	auto pkj = k->point.position - j->point.position;

	auto dir = glm::cross(pij, pkj);

	auto circumcenter = Point(dir);
	//   theta = circumcenter.position.z + radius
	auto theta = acos(circumcenter.position.z) + acos(glm::dot(circumcenter.position, i->point.position));

	if(theta > sweeplineX)
	{
		AddCircleEvent(i, j, k, circumcenter, theta);
	}
}

void PlanetGenerator::AddCircleEvent(Cell* i, Cell* j, Cell* k, Point center, float theta)
{
	auto event = new CircleEvent(new BeachArc(i), new BeachArc(j), new BeachArc(k), center, theta);
	auto pos = lower_bound(circleEventQueue.begin(), circleEventQueue.end(), event, [](const CircleEvent* l, const CircleEvent* r) { return *l < *r; });
	circleEventQueue.emplace(pos, event);
}
