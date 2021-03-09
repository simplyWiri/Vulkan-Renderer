#pragma once
#include "tracy/Tracy.hpp"
#include <vector>
#include <glm/glm/common.hpp>
#include <glm/glm/geometric.hpp>
#include <glm/glm/trigonometric.hpp>
#include <glm/glm/vec2.hpp>
#include <glm/glm/vec3.hpp>

#include "RBtree.h"


namespace World {
	class Planet;
}

namespace World::Generation
{

	struct Point
	{
		Point(float theta, float phi, uint32_t index = 0) : phi(phi), theta(theta), index(index) { UpdatePosition(); }

		Point(glm::vec3 dir, uint32_t index = 0) : index(index)
		{
			float radius = glm::length(dir);

			theta = acos(glm::clamp(dir.z / radius, -1.0f, 1.0f));
			phi = glm::atan(dir.y, dir.x);
			position = dir / radius;
		}

		float SphericalDistance(const Point& other) const
		{
			auto dot = glm::dot(position, other.position);
			return acos(glm::clamp(dot, -1.0f, 1.0f));
		}

		bool operator <(const Point& other) const { return (theta < other.theta) || (theta == other.theta && phi < other.phi); }
		bool operator ==(const Point& other) const { return position == other.position; }
		void UpdatePosition() { position = glm::vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)); }

		float phi;
		float theta;
		uint32_t index;
		glm::vec3 position;
	};

	struct CircleEvent;

	struct BeachArc
	{
		BeachArc(Point cell, uint32_t cellIdx) : site(cell), circleEvent(nullptr), leftEdgeIdx(~0u), rightEdgeIdx(~0u), cellIdx(cellIdx) { }

		Point site;
		CircleEvent* circleEvent;

		uint32_t leftEdgeIdx;
		uint32_t rightEdgeIdx;
		uint32_t cellIdx;

		bool operator <(const BeachArc& other) const { return site < other.site; }

		// returns true if the two arcs intersect, oPhi will be the phi at which the intersection occurs
		bool Intersect(const BeachArc& other, float sweepline, float& oPhi)
		{
			static const float PI = 3.14159265359f;

			oPhi = 0;
			float theta = site.theta;
			float phi = site.phi;

			float otherTheta = other.site.theta;
			float otherPhi = other.site.phi;

			if (theta >= sweepline) // If we are further 'across' the sphere than the sweepline
			{
				if (otherTheta >= sweepline) 
					return false;

				oPhi = phi;
				return true;
			}

			if (otherTheta >= sweepline)
			{
				oPhi = otherPhi;
				return true;
			}

			const auto adjustedPhi = otherPhi - phi;
			
			const auto sinSweeplineX = glm::sin(sweepline);
			const auto cosSweeplineX = glm::cos(sweepline);

			const auto cosTheta = glm::cos(theta);
			const auto sinTheta = glm::sin(theta);
			const auto cosOtherTheta = glm::cos(otherTheta);
			const auto sinOtherTheta = glm::sin(otherTheta);

			const auto a = ((cosSweeplineX - cosOtherTheta) * sinTheta) - ((cosSweeplineX - cosTheta) * sinOtherTheta * glm::cos(adjustedPhi));
			const auto b = -((cosSweeplineX - cosTheta) * sinOtherTheta * glm::sin(adjustedPhi));
			const auto e = (cosTheta - cosOtherTheta) * sinSweeplineX;
			const auto y = std::atan2(a, b);

			oPhi = glm::asin(glm::min(glm::max(e / length( glm::vec2{ a, b }), -1.0f), 1.0f)) - y + phi;
 
	        return true;
		}
	};

	struct CircleEvent
	{
		CircleEvent(const BeachArc& leftArc, Common::LooseOrderedRbTree<BeachArc>::Node* middleArc, const BeachArc& rightArc) : middleArc(middleArc)
		{
			auto leftPos = leftArc.site.position;
			auto middlePos = middleArc->element.site.position;
			auto rightPos = rightArc.site.position;

			auto dir = glm::normalize(cross(leftPos - middlePos, rightPos - middlePos));

			center = dir;
			theta = acos(center.z) + acos(dot(center, middlePos));
		}

		CircleEvent(Common::LooseOrderedRbTree<BeachArc>::Node* j, glm::vec3 center, float theta) : middleArc(j), center(center), theta(theta)
		{
		}

		Common::LooseOrderedRbTree<BeachArc>::Node* middleArc;

		glm::vec3 center; // Circumcenter
		float theta; // Lowest point on circle

		bool operator <(const CircleEvent& other) const { return theta < other.theta; }
	};

	class PlanetGenerator
	{
		friend class Planet;

	public:

		explicit PlanetGenerator(int points, bool random);
		~PlanetGenerator();

		std::vector<glm::vec3> vertices;
		float sweepline = 0;

		Planet* planet;

		std::vector<Point> siteEventQueue; // We are reaching a new point we haven't encountered yet
		std::vector<CircleEvent*> circleEventQueue; // One of the parabolas we have drawn for an existing (processed) point has disappeared
		Common::LooseOrderedRbTree<BeachArc> beach{nullptr};

		void Step(float maxDeltaX);

		bool Finished() { return siteEventQueue.empty() && circleEventQueue.empty(); }

		Planet* RetrievePlanet() const { return planet; }

	private:
		const float PI = 3.14159265359f;

		void GeneratePoints(int numPoints, bool random = false);
		Point GeneratePoint(int index, int numPoints);
		void InitialiseEvents();

		void HandleSiteEvent(const Point& event);
		void HandleCircleEvent(CircleEvent* event);
		void PopulateEdges(BeachArc& prev, BeachArc& succ, glm::vec3 eventCenter);
		void FinishEdges(const BeachArc& arc, uint32_t vertexId);

		void CheckForValidCircleEvent(const BeachArc& i, Common::LooseOrderedRbTree<BeachArc>::Node* j, const BeachArc& k, float sweeplineX, bool siteEvent = false);


		void AddCircleEvent(Common::LooseOrderedRbTree<BeachArc>::Node* j, glm::vec3 center, float theta);
		void RemoveCircleEvent(CircleEvent* event);
		Common::LooseOrderedRbTree<BeachArc>::Node* FindArcOnBeach(const Point& site);
	};
}
