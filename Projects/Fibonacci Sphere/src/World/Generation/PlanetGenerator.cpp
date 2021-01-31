#include "PlanetGenerator.h"
#include "Renderer/Memory/Allocator.h"
#include "Renderer/Memory/Buffer.h"
#include <algorithm>
#include <Renderer\Resources\Vertex.h>



#include "../Planet.h"
#include "Renderer/RenderGraph/GraphContext.h"
#include "Renderer/VulkanObjects/DescriptorSet.h"
#include "Renderer/VulkanObjects/Pipeline.h"

namespace World::Generation
{
	PlanetGenerator::PlanetGenerator()
	{
		planet = new Planet();

		GeneratePoints(5000);
		InitialiseEvents();
	}

	void PlanetGenerator::Step(float maxDeltaX)
	{
		if (!Finished())
		{
			auto nextX = sweepline + maxDeltaX;

			// if site event queue is not empty, and the site is before any events in the circle queue
			if (!siteEventQueue.empty() && (circleEventQueue.empty() || siteEventQueue.front()->theta < circleEventQueue.front()->theta))
			{
				auto siteEvent = siteEventQueue.front();

				if (siteEvent->theta <= nextX)
				{
					sweepline = siteEvent->theta;
					HandleSiteEvent(siteEvent);
					siteEventQueue.erase(siteEventQueue.begin());
				}
				else { sweepline = nextX; }
			}
			else if (!circleEventQueue.empty())
			{
				auto circleEvent = circleEventQueue.front();

				if (circleEvent->theta <= nextX)
				{
					sweepline = circleEvent->theta;
					HandleCircleEvent(circleEvent);
					circleEventQueue.erase(circleEventQueue.end());
				}
				else { sweepline = nextX; }
			}
			else { sweepline = nextX; }
		}
		
	}

	// http://extremelearning.com.au/evenly-distributing-points-on-a-sphere/ # Lattice 3
	void PlanetGenerator::GeneratePoints(int numPoints)
	{
		planet->cells.reserve(numPoints);
		cells.reserve(numPoints);
		
		for (auto i = 0; i < numPoints; i++)
		{
			auto p = GeneratePoint(i, numPoints); // returns coordinates as x, y

			auto theta = acos(2 * p.theta - 1) - PI / 2;
			auto phi = 2 * PI * p.phi;

			auto x = cos(theta) * cos(phi);
			auto y = cos(theta) * sin(phi);
			auto z = sin(theta);


			cells.emplace_back(new Point( {x, y, z}));
			planet->cells.emplace_back( Planet::VoronoiCell{ {x, y, z }, {} });
		}
		
	}

	Point PlanetGenerator::GeneratePoint(int index, int numPoints)
	{
		const float GOLDEN_RATIO = 1.61803398875f;

		if (index == 0) return { 0.0f, 0.0f };
		if (index == numPoints - 1) return { 1.0f, 0.0f };

		auto x = (index + 3.5f) / (numPoints + 6);
		auto y = index / GOLDEN_RATIO;

		return { x, y };
	}

	// Sort the points P, and insert into the site event queue
	void PlanetGenerator::InitialiseEvents()
	{
		std::sort(cells.begin(), cells.end(), [](Point* a, Point* b) { return *a < *b; });

		siteEventQueue = std::vector<Point*>(cells.size());
		std::copy(cells.begin(), cells.end(), siteEventQueue.begin());
	}

	void PlanetGenerator::HandleSiteEvent(Point* event)
	{
		const float PI = 3.14159265359f;

		if (beach.Empty())
		{
			// Insert p into the skiplist, set the previous and next pointers to point to itself
			beach.Insert(event->phi, new BeachArc(event));
		}
		else if (beach.Size() == 1)
		{
			// Append p to the skiplist, and reset pointers correspondingly
			beach.Insert(event->phi, new BeachArc(event));
		}
		else
		{
			// Search the skiplist from the reference position for the arc which will intersect with the great circle
			// which goes through `event` and the north pole
			auto arc = beach.GetHead()->right[0];
			while (arc != NULL)
			{
				auto prevArc = arc->left[0];
				auto nextArc = arc->right[0];

				Point previousArcIntersectPoint(0,0);
				float phiStart = arc->value->cell->phi - PI;

				if (prevArc != NULL && arc->value->Intersect(*prevArc->value, sweepline, previousArcIntersectPoint)) { phiStart = previousArcIntersectPoint.phi; }


				Point nextArcIntersectPoint(0,0);
				float phiEnd = arc->value->cell->phi + PI;

				if (nextArc != NULL && arc->value->Intersect(*nextArc->value, sweepline, nextArcIntersectPoint)) { phiEnd = nextArcIntersectPoint.phi; }

				if (!(phiStart <= phiEnd && phiStart <= event->phi && event->phi <= phiEnd) || (event->phi < phiEnd || event->phi > phiStart)) break;

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
		beach.Erase(event->j->cell->phi);
		std::remove(circleEventQueue.begin(), circleEventQueue.end(), event);

		// Add thing to vertex list `event->center`

		// Remove all circle events associated with the triples (1, i, j) and (j, k, 2)

		// Check the two new triples of consecutive arcs (1, i, k) and (i, k, 2)
		// CheckForValidCircleEvent(1, i, k);
		// CheckForValidCircleEvent(i, k, 2);
	}

	void PlanetGenerator::CheckForValidCircleEvent(Point* i, Point* j, Point* k, float sweeplineX)
	{
		auto pij = i->position - j->position;
		auto pkj = k->position - j->position;

		auto dir = glm::cross(pij, pkj);

		auto circumcenter = Point(dir);
		//   theta = circumcenter.position.z + radius
		auto theta = acos(circumcenter.position.z) + acos(glm::dot(circumcenter.position, i->position));

		if (theta > sweeplineX) { AddCircleEvent(i, j, k, circumcenter, theta); }
	}

	void PlanetGenerator::AddCircleEvent(Point* i, Point* j, Point* k, Point center, float theta)
	{
		auto event = new CircleEvent(new BeachArc(i), new BeachArc(j), new BeachArc(k), center, theta);
		auto pos = lower_bound(circleEventQueue.begin(), circleEventQueue.end(), event, [](const CircleEvent* l, const CircleEvent* r) { return *l < *r; });
		circleEventQueue.emplace(pos, event);
	}
}
