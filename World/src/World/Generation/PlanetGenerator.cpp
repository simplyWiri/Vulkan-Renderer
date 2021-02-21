#include "PlanetGenerator.h"
#include "Renderer/Memory/Allocator.h"
#include <algorithm>
#include <Tracy.hpp>
#include <Renderer\Resources\Vertex.h>
#include <random>


#include "../Planet.h"

namespace World::Generation
{
	PlanetGenerator::PlanetGenerator(int points, bool random)
	{
		planet = new Planet();
		planet->cells.reserve(points);

		GeneratePoints(points, random);
		InitialiseEvents();
	}

	PlanetGenerator::~PlanetGenerator()
	{
		for (auto cell : cells) 
			delete cell;

		delete planet;
	}

	void PlanetGenerator::Step(float maxDeltaX)
	{
		if (!Finished())
		{
			auto final = sweepline + maxDeltaX;
			auto nextX = sweepline;
			while (nextX < final)
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

				nextX = sweepline;
			}

			// Find the remaining two half edges which are not completed, and connect them.
			if (Finished())
			{
				for (int i = 0; i < planet->halfEdges.size(); i++)
				{
					auto& edge = planet->halfEdges[i];
					if (edge.finished) continue;

					for (int j = i + 1; j < planet->halfEdges.size(); j++)
					{
						auto& otherEdge = planet->halfEdges[j];
						if (otherEdge.finished) continue;


						otherEdge.endIndex = edge.beginIndex;
						edge.endIndex = otherEdge.beginIndex;

						otherEdge.finished = true;
						edge.finished = true;

						return;
					}
				}

				sweepline = 2 * PI;
			}
		}
	}

	// http://extremelearning.com.au/evenly-distributing-points-on-a-sphere/ # Lattice 3
	void PlanetGenerator::GeneratePoints(int numPoints, bool random)
	{
		planet->cells.reserve(numPoints);
		cells.reserve(numPoints);

		if (random)
		{
			for (auto i = 0; i < numPoints; i++)
			{
				auto u = rand() / (float)RAND_MAX;
				auto v = rand() / (float)RAND_MAX;

				cells.emplace_back(new Point{ acos(2 * v - 1), 2 * PI * u });
			}
		}
		else
		{
			for (auto i = 0; i < numPoints; i++)
			{
				auto p = GeneratePoint(i, numPoints); // returns coordinates as x, y

				auto theta = acos(2 * p.theta - 1) - PI / 2;
				auto phi = 2 * PI * p.phi;

				auto x = cos(theta) * cos(phi);
				auto y = cos(theta) * sin(phi);
				auto z = sin(theta);

				cells.emplace_back(new Point({ x, y, z }));
			}
		}
	}

	Point PlanetGenerator::GeneratePoint(int index, int numPoints)
	{
		const float GOLDEN_RATIO = 1.61803398875f;

		if (index == 0) return { 0.00001f, 0.0f };
		if (index == numPoints - 1) return { 0.99999f, 0.0f };

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

	Point PhiToPoint(Point arc, float phi, float sweepline)
	{
		float a = cos(arc.theta) - cos(sweepline);
		float b = sin(sweepline) - sin(arc.theta) * cos(phi - arc.phi);
		return Point(atan2(a, b), phi);
	}

	void PlanetGenerator::HandleSiteEvent(Point* event)
	{
		if (beach.Count() <= 1)
		{
			auto* iter = beach.Append(new BeachArc(event, planet->cells.size()));
			planet->cells.emplace_back(VoronoiCell{ event->position });

			if (beach.Count() == 2) // We need to stitch the first & second beach entries with a little more care.
			{
				auto root = beach.GetRoot();

				iter->element->leftEdgeIdx = root->element->rightEdgeIdx;
				iter->element->rightEdgeIdx = root->element->leftEdgeIdx;

				auto vertex = PhiToPoint(*root->element->site, event->phi, sweepline);

				PopulateEdges(root->element, iter->element, vertex.position);
				PopulateEdges(iter->element, root->element, vertex.position);
			}
		}
		else
		{
			// Search the skiplist from the reference position for the arc which will intersect with the great circle
			// which goes through `event` and the north pole, a log(n) lookup through an rbtree
			auto* curr = FindArcOnBeach(event);
			if (curr == nullptr) return;

			auto* pred = curr->Predecessor() ? curr->Predecessor() : beach.Last();
			auto* succ = curr->Successor() ? curr->Successor() : beach.First();

			// false alarm, remove the event from the circleEventQueue
			if (curr->element->circleEvent)
			{
				RemoveCircleEvent(curr->element->circleEvent);
				curr->element->circleEvent = nullptr;
			}

			// Duplicate arc a and insert the new arc for `event` between them in the skiplist
			auto* duplicatedArc = new BeachArc(curr->element->site, curr->element->cellIdx);
			auto* newArc = new BeachArc(event, static_cast<uint32_t>(planet->cells.size()));
			duplicatedArc->rightEdgeIdx = curr->element->rightEdgeIdx;


			planet->cells.emplace_back(VoronoiCell{ event->position });

			auto* newArcLoc = beach.Insert(newArc, curr);
			auto* duplicatedArcLoc = beach.Insert(duplicatedArc, newArcLoc);

			auto vertex = PhiToPoint(*curr->element->site, event->phi, sweepline);

			PopulateEdges(curr->element, newArcLoc->element, vertex.position);
			PopulateEdges(newArcLoc->element, duplicatedArcLoc->element, vertex.position);

			// check (p2, pj', pi) and (pi, pj'', p3) for valid circle events
			CheckForValidCircleEvent(pred->element, curr, newArcLoc->element, sweepline, true);
			CheckForValidCircleEvent(newArcLoc->element, duplicatedArcLoc, succ->element, sweepline, true);
		}
	}

	void PlanetGenerator::HandleCircleEvent(CircleEvent* event)
	{
		if (beach.Count() == 2) return;
		// Remove the site `j` which represents the disappearing arc from the skiplist and remove the circle event from the circleQueue
		auto j = event->middleArc;
		auto i = j->Predecessor() ? j->Predecessor() : beach.Last();
		auto k = j->Successor() ? j->Successor() : beach.First();
		auto arc1 = i->Predecessor() ? i->Predecessor()->element : beach.Last()->element;
		auto arc2 = k->Successor() ? k->Successor()->element : beach.First()->element;

		// Add our event center as an edge
		// Populate the half edges extending from the center
		PopulateEdges(i->element, k->element, event->center);
		// Finish the edges extending to the center
		FinishEdges(j->element, static_cast<uint32_t>(planet->edgeVertices.size() - 1));

		// Remove the fading arc from the beach
		j->element->circleEvent = nullptr;
		beach.Remove(j);

		// Remove all circle events associated with the triples (1, i, j) and (j, k, 2)
		if (i->element->circleEvent)
		{
			RemoveCircleEvent(i->element->circleEvent);
			i->element->circleEvent = nullptr;
		}
		if (k->element->circleEvent)
		{
			RemoveCircleEvent(k->element->circleEvent);
			k->element->circleEvent = nullptr;
		}

		// Check the two new triples of consecutive arcs (1, i, k) and (i, k, 2)
		CheckForValidCircleEvent(arc1, i, k->element, sweepline);
		CheckForValidCircleEvent(i->element, k, arc2, sweepline);
	}

	void PlanetGenerator::PopulateEdges(BeachArc* prev, BeachArc* succ, glm::vec3 eventCenter)
	{
		// Add event center to vertex list `event->centers`
		auto edgeVertexId = static_cast<uint32_t>(planet->edgeVertices.size());
		planet->edgeVertices.emplace_back(eventCenter);

		// voronoi
		auto halfEdgeID = static_cast<uint32_t>(planet->halfEdges.size());
		auto halfEdge = HalfEdge{ false, edgeVertexId, ~0u, halfEdgeID, };
		planet->halfEdges.emplace_back(halfEdge);

		// delanuay
		auto size = static_cast<uint32_t>(planet->delanuayCells.size());
		planet->delanuayCells.emplace_back(prev->site->position);
		planet->delanuayCells.emplace_back(succ->site->position);
		planet->delanuayEdges.emplace_back(HalfEdge{ true, size, size + 1 });

		planet->cells[prev->cellIdx].edges.emplace_back(halfEdgeID);
		planet->cells[succ->cellIdx].edges.emplace_back(halfEdgeID);

		prev->rightEdgeIdx = succ->leftEdgeIdx = halfEdgeID;
	}

	void PlanetGenerator::FinishEdges(BeachArc* arc, uint32_t vertexId)
	{
		if (arc->leftEdgeIdx != ~0u) // We have other issues if ~0u is an actual index.
		{
			auto& edge = planet->halfEdges[arc->leftEdgeIdx];
			if (edge.finished == false)
			{
				edge.endIndex = vertexId;
				edge.finished = true;
			}
		}

		if (arc->rightEdgeIdx != ~0u)
		{
			auto& edge = planet->halfEdges[arc->rightEdgeIdx];
			if (edge.finished == false)
			{
				edge.endIndex = vertexId;
				edge.finished = true;
			}
		}
	}


	void PlanetGenerator::CheckForValidCircleEvent(BeachArc* i, Common::LooseOrderedRbTree<BeachArc*>::Node* j, BeachArc* k, float sweeplineX, bool siteEvent)
	{
		if (!siteEvent && (i->site == j->element->site || j->element->site == k->site || i->site == k->site)) return;

		auto pij = i->site->position - j->element->site->position;
		auto pkj = k->site->position - j->element->site->position;

		auto dir = glm::cross(pij, pkj);
		auto circumcenter = glm::normalize(dir);
		auto theta = acos(circumcenter.z) + acos(glm::dot(circumcenter, j->element->site->position));

		// not sure why this breaks the code... ?
		//if(theta > sweepline) 
		AddCircleEvent(i, j, k, circumcenter, theta);
	}

	void PlanetGenerator::AddCircleEvent(BeachArc* i, Common::LooseOrderedRbTree<BeachArc*>::Node* j, BeachArc* k, glm::vec3 center, float theta)
	{
		auto* event = new CircleEvent(i, j, k, center, theta);
		j->element->circleEvent = event;
		auto pos = upper_bound(circleEventQueue.begin(), circleEventQueue.end(), event, [](const CircleEvent* l, const CircleEvent* r) { return *l < *r; });
		circleEventQueue.emplace(pos, event);
	}

	void PlanetGenerator::RemoveCircleEvent(CircleEvent* event)
	{
		if (event == nullptr) return;

		if (circleEventQueue.size() == 1) circleEventQueue.erase(circleEventQueue.begin());
		else
		{
			auto it = lower_bound(circleEventQueue.begin(), circleEventQueue.end(), event, [](const CircleEvent* l, const CircleEvent* r) { return *l < *r; });
			if (*it != event)
			{
				//delete event;
				return;
			}
			circleEventQueue.erase(it);
		}

		delete event;
	}

	// Based on: https://github.com/kelvin13/voronoi/blob/master/sources/game/lib/voronoi.swift#L1061-L1185

	Common::LooseOrderedRbTree<BeachArc*>::Node* PlanetGenerator::FindArcOnBeach(Point* site)
	{
		auto* first = beach.First();
		auto* last = beach.Last();
		auto* curr = beach.GetRoot();

		if (first == last) return nullptr;

		auto locationPhi = site->phi;

		float shift;
		last->element->Intersect(*first->element, sweepline, shift);
		shift *= -1;

		auto adjust = [&](float phi)
		{
			static const float divisor = PI * 2;
			auto v = phi + shift + (6 * PI);

			return fmodf(v, divisor);
		};

		locationPhi = adjust(locationPhi);

		auto pred = curr, succ = curr;

		float phiStart = std::numeric_limits<float>::min(), phiEnd = std::numeric_limits<float>::min();

		while (true)
		{
			pred = curr->Predecessor();
			succ = curr->Successor();

			if (pred != nullptr && curr->left != nullptr && pred->element->Intersect(*curr->element, sweepline, phiStart) && locationPhi < adjust(phiStart))
			{
				curr = curr->left;
				continue;
			}

			if (succ != nullptr && curr->right != nullptr && curr->element->Intersect(*succ->element, sweepline, phiEnd) && locationPhi > adjust(phiEnd))
			{
				curr = curr->right;
				continue;
			}

			break;
		}

		return curr;
	}
}
