#include <random>

#include "PlanetGenerator.h"
#include "../Planet.h"

namespace World::Generation
{
	
	template <class T>
	PlanetGenerator<T>::PlanetGenerator(int points, bool random)
	{
		planet = new Planet();
		planet->cells.reserve(points);

		GeneratePoints(points, random);
		InitialiseEvents();
	}

	template <class T>
	PlanetGenerator<T>::~PlanetGenerator()
	{
		delete planet;
	}

	template <class T>
	void PlanetGenerator<T>::Step(float maxDeltaX)
	{
		if (!Finished())
		{
			auto final = sweepline + maxDeltaX;
			auto nextX = sweepline;
			while (nextX < final)
			{
				nextX = final;

				// if site event queue is not empty, and the site is before any events in the circle queue
				if (!siteEventQueue.empty() && (circleEventQueue.empty() || siteEventQueue.front().theta < circleEventQueue.front()->theta))
				{
					auto& siteEvent = siteEventQueue.front();

					if (siteEvent.theta <= nextX)
					{
						sweepline = siteEvent.theta;
						HandleSiteEvent(siteEvent);
						siteEventQueue.erase(siteEventQueue.begin());
					}
					else { sweepline = nextX; }
				}
				else if (!circleEventQueue.empty())
				{
					auto* circleEvent = circleEventQueue.front();
					
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
	template <class T>
	void PlanetGenerator<T>::GeneratePoints(int numPoints, bool random)
	{
		planet->cells.reserve(numPoints);
		siteEventQueue.reserve(numPoints);

		if (random)
		{
			static auto seed = 500;
			thread_local static std::mt19937 prng( seed );
			
			for (auto i = 0; i < numPoints; i++)
			{
				auto u = std::uniform_real_distribution<T>(0, 1) (prng);
				auto v = std::uniform_real_distribution<T>(0, 1) (prng);

				siteEventQueue.emplace_back(acos(2 * v - 1), 2 * PI * u);
			}
		}
		else
		{
			for (auto i = 0; i < numPoints; i++)
			{
				auto p = Point::GeneratePoint(i, numPoints); // returns coordinates as x, y

				auto theta = acos(2 * p.theta - 1) - PI / 2;
				auto phi = 2 * PI * p.phi;

				auto x = cos(theta) * cos(phi);
				auto y = cos(theta) * sin(phi);
				auto z = sin(theta);

				siteEventQueue.emplace_back(glm::vec3{ x, y, z });
			}
		}
	}

	// Sort the points P, and insert into the site event queue
	template <class T>
	void PlanetGenerator<T>::InitialiseEvents()
	{
		std::sort(siteEventQueue.begin(), siteEventQueue.end(), [](const auto& a, const auto& b) { return a < b; });

		for(int i = 0; i < siteEventQueue.size(); i++)
		{
			siteEventQueue[i].index = i;
		}
	}

	template <class T>
	void PlanetGenerator<T>::HandleSiteEvent(const Point& event)
	{
		if (beach.Count() <= 1)
		{
			auto* iter = beach.Append(BeachArc(event, planet->cells.size()));
			planet->cells.emplace_back(VoronoiCell{ event.position });

			if (beach.Count() == 2) // We need to stitch the first & second beach entries with a little more care.
			{
				auto* root = beach.GetRoot();

				iter->element.leftEdgeIdx = root->element.rightEdgeIdx;
				iter->element.rightEdgeIdx = root->element.leftEdgeIdx;

				const auto vertex = Point::PhiToPoint(root->element.site, event.phi, sweepline);

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

			const auto* pred = curr->Predecessor() ? curr->Predecessor() : beach.Last();
			const auto* succ = curr->Successor() ? curr->Successor() : beach.First();

			// false alarm, remove the event from the circleEventQueue
			if (curr->element.circleEvent)
			{
				RemoveCircleEvent(curr->element.circleEvent);
				curr->element.circleEvent = nullptr;
			}

			// Duplicate arc a and insert the new arc for `event` between them in the skiplist
			auto duplicatedArc = BeachArc(curr->element.site, curr->element.cellIdx);
			const auto newArc = BeachArc(event, static_cast<uint32_t>(planet->cells.size()));
			
			duplicatedArc.rightEdgeIdx = curr->element.rightEdgeIdx;
			
			planet->cells.emplace_back(VoronoiCell{ event.position });

			auto* newArcLoc = beach.Insert(newArc, curr);
			auto* duplicatedArcLoc = beach.Insert(duplicatedArc, newArcLoc);

			const auto vertex = Point::PhiToPoint(curr->element.site, event.phi, sweepline);

			PopulateEdges(curr->element, newArcLoc->element, vertex.position);
			PopulateEdges(newArcLoc->element, duplicatedArcLoc->element, vertex.position);

			// check (p2, pj', pi) and (pi, pj'', p3) for valid circle events
			CheckForValidCircleEvent(pred->element, curr, newArcLoc->element, sweepline, true);
			CheckForValidCircleEvent(newArcLoc->element, duplicatedArcLoc, succ->element, sweepline, true);
		}
	}

	template <class T>
	void PlanetGenerator<T>::HandleCircleEvent(CircleEvent* event)
	{
		if (beach.Count() == 2) return;
		// Remove the site `j` which represents the disappearing arc from the skiplist and remove the circle event from the circleQueue
		auto* j = event->middleArc;
		auto* i = j->Predecessor() ? j->Predecessor() : beach.Last();
		auto* k = j->Successor() ? j->Successor() : beach.First();
		auto& arc1 = i->Predecessor() ? i->Predecessor()->element : beach.Last()->element;
		auto& arc2 = k->Successor() ? k->Successor()->element : beach.First()->element;

		// Add our event center as an edge
		// Populate the half edges extending from the center
		PopulateEdges(i->element, k->element, event->center);
		// Finish the edges extending to the center
		FinishEdges(j->element, static_cast<uint32_t>(planet->edgeVertices.size() - 1));

		// Remove the fading arc from the beach
		beach.Remove(j);

		// Remove all circle events associated with the triples (1, i, j) and (j, k, 2)
		if (i->element.circleEvent)
		{
			RemoveCircleEvent(i->element.circleEvent);
			i->element.circleEvent = nullptr;
		}
		if (k->element.circleEvent)
		{
			RemoveCircleEvent(k->element.circleEvent);
			k->element.circleEvent = nullptr;
		}

		// Check the two new triples of consecutive arcs (1, i, k) and (i, k, 2)
		CheckForValidCircleEvent(arc1, i, k->element, sweepline);
		CheckForValidCircleEvent(i->element, k, arc2, sweepline);
	}

	template <class T>
	void PlanetGenerator<T>::PopulateEdges(BeachArc& prev, BeachArc& succ, glm::vec3 eventCenter)
	{
		// Add event center to vertex list `event->centers`
		auto edgeVertexId = static_cast<uint32_t>(planet->edgeVertices.size());
		planet->edgeVertices.emplace_back(eventCenter);

		// voronoi
		auto halfEdgeID = static_cast<uint32_t>(planet->halfEdges.size());
		auto halfEdge = HalfEdge{ false, edgeVertexId, ~0u, halfEdgeID };
		planet->halfEdges.emplace_back(halfEdge);

		// delanuay
		planet->delanuayEdges.emplace_back(HalfEdge{ true, prev.site.index, succ.site.index });
		planet->cells[prev.site.index].AddDelEdge(planet->delanuayEdges.size() - 1);
		planet->cells[succ.site.index].AddDelEdge(planet->delanuayEdges.size() - 1);

		planet->cells[prev.cellIdx].AddEdge(halfEdgeID);
		planet->cells[succ.cellIdx].AddEdge(halfEdgeID);

		prev.rightEdgeIdx = succ.leftEdgeIdx = halfEdgeID;
	}

	template <class T>
	void PlanetGenerator<T>::FinishEdges(const BeachArc& arc, uint32_t vertexId)
	{
		if (arc.leftEdgeIdx != ~0u) // We have other issues if ~0u is an actual index.
		{
			auto& edge = planet->halfEdges[arc.leftEdgeIdx];
			if (edge.finished == false)
			{
				edge.endIndex = vertexId;
				edge.finished = true;
			}
		}

		if (arc.rightEdgeIdx != ~0u)
		{
			auto& edge = planet->halfEdges[arc.rightEdgeIdx];
			if (edge.finished == false)
			{
				edge.endIndex = vertexId;
				edge.finished = true;
			}
		}
	}


	template <class T>
	void PlanetGenerator<T>::CheckForValidCircleEvent(const BeachArc& i, Common::LooseOrderedRbTree<BeachArc>::Node* j, const BeachArc& k, T sweeplineX, bool siteEvent)
	{
		if (!siteEvent && (i.site == j->element.site || j->element.site == k.site || i.site == k.site)) return;

		auto pij = i.site.position - j->element.site.position;
		auto pkj = k.site.position - j->element.site.position;

		auto dir = glm::cross(pij, pkj);
		auto circumcenter = glm::normalize(dir);
		auto theta = acos(circumcenter.z) + acos(glm::dot(circumcenter, j->element.site.position));

		// not sure why this breaks the code... ?
		//if(theta > sweepline) 
		AddCircleEvent(j, circumcenter, theta);
	}

	template <class T>
	void PlanetGenerator<T>::AddCircleEvent(Common::LooseOrderedRbTree<BeachArc>::Node* j, glm::vec3 center, T theta)
	{
		auto* event = new CircleEvent(j, center, theta);
		j->element.circleEvent = event;
		auto pos = upper_bound(circleEventQueue.begin(), circleEventQueue.end(), event, [](const CircleEvent* l, const CircleEvent* r) { return *l < *r; });
		circleEventQueue.emplace(pos, event);
	}

	template <class T>
	void PlanetGenerator<T>::RemoveCircleEvent(CircleEvent* event)
	{
		if (event == nullptr) return;

		if (circleEventQueue.size() == 1) circleEventQueue.erase(circleEventQueue.begin());
		else
		{
			auto it = lower_bound(circleEventQueue.begin(), circleEventQueue.end(), event, [](const CircleEvent* l, const CircleEvent* r) { return *l < *r; });
			if (*it != event)
			{
				return;
			}
			circleEventQueue.erase(it);
		}

		delete event;
	}
}
