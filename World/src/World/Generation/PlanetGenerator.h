#pragma once
#include <ostream>
#include <vector>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "RBtree.h"


namespace World {
	class Planet;
}

namespace World::Generation
{

	struct Point
	{
		Point(float theta, float phi) : phi(phi), theta(theta) { UpdatePosition(); }

		Point(glm::vec3 dir)
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


		static Point PhiToPoint(float phi, float x, float theta, float otherPhi)
		{
			if (theta >= x)
			{
				return Point(x, phi); // could be any point on the line segment
			}
			else
			{
				auto a = - (glm::sin(theta) * glm::cos(phi - otherPhi) - glm::sin(x));
				auto b = - (glm::cos(x) - glm::cos(theta));
				auto theta = glm::atan(b, a);
				return Point(theta, phi);
			}
		}

		float phi;
		float theta;
		glm::vec3 position;
	};

	struct CircleEvent;

	struct BeachArc
	{
		BeachArc(Point* cell) : cell(cell), circleEvent(nullptr) { }

		Point* cell;
		CircleEvent* circleEvent;

		bool operator <(const BeachArc& other) const { return *cell < *other.cell; }

		friend std::ostream& operator<<(std::ostream& os, const BeachArc& a)
		{
		    os << a.cell->phi;
		    return os;
		}
		
		// returns true if the two arcs intersect, location will be a ref to the location at which the intersection occurs
		bool Intersect(const BeachArc& other, float sweepline, float& oPhi)
		{
			static const float PI = 3.14159265359f;

			oPhi = 0;
			float theta = cell->theta;
			float phi = cell->phi;

			float otherTheta = other.cell->theta;
			float otherPhi = other.cell->phi;

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

			const auto a = ((cosSweeplineX - cosOtherTheta) * sinTheta) - ((cosSweeplineX - cosTheta) * sinOtherTheta * std::cos(adjustedPhi));
			const auto b = -((cosSweeplineX - cosTheta) * sinOtherTheta * std::sin(adjustedPhi));
			const auto e = (cosTheta - cosOtherTheta) * sinSweeplineX;
			const auto y = std::atan2(a, b);

			oPhi = glm::asin(glm::min(glm::max(e / length( glm::vec2{ a, b }), -1.0f), 1.0f)) - y + phi;
 
	        return true;
		}
	};

	struct CircleEvent
	{
		CircleEvent(BeachArc* leftArc, Common::LooseOrderedRbTree<BeachArc*>::Node* middleArc, BeachArc* rightArc) : leftArc(leftArc), middleArc(middleArc),  rightArc(rightArc)
		{
			auto leftPos = leftArc->cell->position;
			auto middlePos = middleArc->element->cell->position;
			auto rightPos = rightArc->cell->position;

			auto dir = glm::normalize(cross(leftPos - middlePos, rightPos - middlePos));

			center = Point(dir);
			theta = acos(center.position.z) + acos(dot(center.position, middlePos));
		}

		CircleEvent(BeachArc* i, Common::LooseOrderedRbTree<BeachArc*>::Node* j, BeachArc* k, Point center, float theta) : leftArc(i), middleArc(j), rightArc(k) , center(center), theta(theta)
		{
		}

		BeachArc* leftArc;
		Common::LooseOrderedRbTree<BeachArc*>::Node* middleArc;
		BeachArc* rightArc;
		bool valid;

		Point center = {0,0}; // Circumcenter
		float theta; // Lowest point on circle

		bool operator <(const CircleEvent& other) const { return theta < other.theta; }
	};

	class PlanetGenerator
	{
		friend class Planet;

	public:

		explicit PlanetGenerator();
		~PlanetGenerator();

		std::vector<glm::vec3> vertices;
		float sweepline = 0;

		Planet* planet;
		std::vector<Point*> cells;

		std::vector<Point*> siteEventQueue; // We are reaching a new point we haven't encountered yet
		std::vector<CircleEvent*> circleEventQueue; // One of the parabolas we have drawn for an existing (processed) point has disappeared
		Common::LooseOrderedRbTree<BeachArc*> beach{nullptr};

		void Step(float maxDeltaX);

		bool Finished() { return siteEventQueue.empty() && circleEventQueue.empty(); }

		Planet* RetrievePlanet() const { return planet; }

	private:
		const float PI = 3.14159265359f;

		void GeneratePoints(int numPoints);
		Point GeneratePoint(int index, int numPoints);
		void InitialiseEvents();

		void HandleSiteEvent(Point* event);
		void HandleCircleEvent(CircleEvent* event);

		void CheckForValidCircleEvent(BeachArc* i, Common::LooseOrderedRbTree<BeachArc*>::Node* j, BeachArc* k, float sweeplineX);


		void AddCircleEvent(BeachArc* i, Common::LooseOrderedRbTree<BeachArc*>::Node* j, BeachArc* k, Point center, float theta);
		void RemoveCircleEvent(CircleEvent* event);
		Common::LooseOrderedRbTree<BeachArc*>::Node* FindArcOnBeach(Point* site);
	};
}
