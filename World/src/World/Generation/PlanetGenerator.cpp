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

		GeneratePoints(500);
		InitialiseEvents();
	}

	PlanetGenerator::~PlanetGenerator()
	{
		for (auto cell : cells) delete cell;

		delete planet;
	}

	void PlanetGenerator::Step(float maxDeltaX)
	{
		if (!Finished())
		{
			auto final = sweepline + maxDeltaX;
			auto nextX = 0.0f;
			while (nextX != final)
			{
				nextX = final;

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
						RemoveCircleEvent(circleEvent);
					}
					else { sweepline = nextX; }
				}
				else { sweepline = nextX; }
			}
		} else
		{
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

			cells.emplace_back(new Point({ x, y, z }));
			planet->cells.emplace_back(Planet::VoronoiCell{ { x, y, z }, {} });
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
		if (beach.Count() <= 1)
		{
			beach.Append(new BeachArc(event));
		}
		else
		{
			// Search the skiplist from the reference position for the arc which will intersect with the great circle
			// which goes through `event` and the north pole

			auto* curr = FindArcOnBeach(event);
			if(curr == nullptr) return;
			
			auto* pred = curr->Predecessor() ? curr->Predecessor() : beach.Last();
			auto* succ = curr->Successor() ? curr->Successor() : beach.First();
			
			// if ( arc.circleEvent != null ) // false alarm, remove the event from the circleEventQueue
			if (curr->element->circleEvent) RemoveCircleEvent(curr->element->circleEvent);

			// Duplicate arc a and insert the new arc for `event` between them in the skiplist
			auto* duplicatedArc = new BeachArc(curr->element->cell);
			auto* newArc = new BeachArc(event);

			auto* newArcLoc = beach.Insert(newArc, curr);
			auto* duplicatedArcLoc = beach.Insert(duplicatedArc, newArcLoc);

			// check (p2, pj, pi) and (pi, pj, p3) for valid circle events
			CheckForValidCircleEvent(pred->element, newArcLoc, curr->element, sweepline);
			CheckForValidCircleEvent(curr->element, duplicatedArcLoc, succ->element, sweepline);
		}
	}

	void PlanetGenerator::HandleCircleEvent(CircleEvent* event)
	{
		// Remove the site `j` which represents the disappearing arc from the skiplist and remove the circle event from the circleQueue
		auto j = event->middleArc;
		auto i = j->Predecessor() ? j->Predecessor() : beach.Last();
		auto k = j->Successor() ? j->Successor() : beach.First();
		auto arc1 = i->Predecessor() ? i->Predecessor()->element : beach.Last()->element;
		auto arc2 = k->Successor()  ? k->Successor()->element : beach.First()->element;

		auto eventCenter = event->center.position;
		
		// Remove the fading arc from the beach
		j->element->circleEvent = nullptr;
		beach.Remove(j);

		// Add thing to vertex list `event->center`
		planet->edgeVertices.emplace_back(eventCenter);

		// todo edges
		//planet->halfEdges.emplace_back( );

		// Remove all circle events associated with the triples (1, i, j) and (j, k, 2)
		if (i->element->circleEvent) RemoveCircleEvent(i->element->circleEvent);
		if (k->element->circleEvent) RemoveCircleEvent(k->element->circleEvent);

		// Check the two new triples of consecutive arcs (1, i, k) and (i, k, 2)
		CheckForValidCircleEvent(arc1, i, k->element, sweepline);
		CheckForValidCircleEvent(i->element, k, arc2, sweepline);
	}

	void PlanetGenerator::CheckForValidCircleEvent(BeachArc* i, Common::LooseOrderedRbTree<BeachArc*>::Node* j, BeachArc* k, float sweeplineX)
	{
		if (i->cell == j->element->cell || i->cell == k->cell || k->cell == j->element->cell) return;

		auto pij = i->cell->position - j->element->cell->position;
		auto pkj = k->cell->position - j->element->cell->position;

		auto dir = glm::cross(pij, pkj);

		auto circumcenter = Point(glm::normalize(dir));

		auto theta = acos(circumcenter.position.z) + acos(glm::dot(circumcenter.position, j->element->cell->position));

		// Do not evaluate points when the lowest point is above the sweepline.
		//if (theta > sweeplineX)
		//{
			AddCircleEvent(i, j, k, circumcenter, theta);
		//} 
	}

	void PlanetGenerator::AddCircleEvent(BeachArc* i, Common::LooseOrderedRbTree<BeachArc*>::Node* j, BeachArc* k, Point center, float theta)
	{
		auto* event = new CircleEvent(i, j, k, center, theta);
		j->element->circleEvent = event;
		auto pos = lower_bound(circleEventQueue.begin(), circleEventQueue.end(), event, [](const CircleEvent* l, const CircleEvent* r) { return *l < *r; });
		circleEventQueue.emplace(pos, event);
	}

	void PlanetGenerator::RemoveCircleEvent(CircleEvent* event)
	{
		if (event == nullptr || !event->valid) return;

		auto it = std::find(circleEventQueue.begin(), circleEventQueue.end(), event);
		circleEventQueue.erase(it);

		event->valid = false;
	}

	Common::LooseOrderedRbTree<BeachArc*>::Node* PlanetGenerator::FindArcOnBeach(Point* site)
	{
		auto* first = beach.First();
		auto* last = beach.Last();
		auto* curr = beach.GetRoot();

		if(first == last) return nullptr;

		auto locationPhi = site->phi;

		float shift;
		last->element->Intersect(*first->element, sweepline, shift);
		shift *= -1;
		
		auto adjust = [&](float phi)
		{
			static const float divisor = PI * 2;
			auto v = phi + shift + (6 * PI);

			return fmod(v, divisor);
		};

		locationPhi = adjust(locationPhi);

		auto pred = curr, succ = curr;

		float phiStart = std::numeric_limits<float>::min(), phiEnd = std::numeric_limits<float>::min();

		while (true)
		{
			pred = curr->Predecessor();
			succ = curr->Successor();

			if (pred != nullptr && pred->element->Intersect(*curr->element, sweepline, phiStart) && locationPhi < adjust(phiStart))
			{
				curr = pred;
				continue;
			}
			else if (succ != nullptr && curr->element->Intersect(*succ->element, sweepline, phiEnd) && locationPhi > adjust(phiEnd))
			{
				curr = succ;
				continue;
			}

			break;
		}

		//pred = curr->Predecessor() == nullptr ? last : curr->Predecessor();
		//succ = curr->Successor() == nullptr ? first : curr->Successor();

		//bool valid = pred->element->Intersect(*curr->element, sweepline, phiStart) && curr->element->Intersect(*succ->element, sweepline, phiEnd);

		//if(valid && phiStart < phiEnd && phiStart <= site->phi && site->phi <= phiEnd || phiStart > phiEnd && (phiStart <=site->phi || site->phi <= phiEnd)) 
			return curr;
		/*return nullptr;*/
	}
}
