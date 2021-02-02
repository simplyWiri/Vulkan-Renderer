#pragma once
#include <vector>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include "../Skiplist.h"

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
		BeachArc(Point* cell) : cell(cell) { }

		Point* cell;
		CircleEvent* circleEvent;

		bool operator <(const BeachArc& other) const { return cell->phi < other.cell->phi; }

		// returns true if the two arcs intersect, location will be a ref to the location at which the intersection occurs
		bool Intersect(const BeachArc& other, float sweeplineX, Point& location)
		{
			float theta = cell->theta;
			float phi = cell->phi;

			float otherTheta = cell->theta;
			float otherPhi = cell->phi;

			if (theta >= sweeplineX) // If we are further 'across' the sphere than the sweepline
			{
				if (otherTheta >= sweeplineX) return false;

				location = Point::PhiToPoint(phi, sweeplineX, otherTheta, otherPhi);
				return true;
			}

			if (otherTheta >= sweeplineX)
			{
				location = Point::PhiToPoint(otherPhi, sweeplineX, theta, phi);
				return true;
			}


			auto sinSweeplineX = glm::sin(sweeplineX);
			auto cosSweeplineX = glm::cos(sweeplineX);

			auto cosTheta = glm::cos(theta);
			auto sinTheta = glm::sin(theta);
			auto cosOtherTheta = glm::cos(otherTheta);
			auto sinotherTheta = glm::sin(otherTheta);
			auto cosPhi = glm::cos(phi);
			auto sinPhi = glm::sin(phi);
			auto cosOtherPhi = glm::cos(otherPhi);
			auto sinOtherPhi = glm::sin(otherPhi);

			auto a = ((cosSweeplineX - cosOtherTheta) * sinTheta * cosPhi) - ((cosSweeplineX - cosTheta) * sinotherTheta * cosOtherPhi);
			auto b = ((cosSweeplineX - cosOtherTheta) * sinTheta * sinPhi) - ((cosSweeplineX - cosTheta) * sinotherTheta * sinOtherPhi);
			auto e = (cosTheta - cosOtherTheta) * sinSweeplineX;

			auto length = glm::sqrt(a * a + b * b);

			if (abs(a) > length || abs(b) > length) return false;

			// Todo... figure out how this section works

			auto gamma = glm::atan(a, b);
			auto sin_phi_int_plus_gamma_1 = e / length;
			auto phi_int_plus_gamma_1 = glm::asin(sin_phi_int_plus_gamma_1);
			auto pA = phi_int_plus_gamma_1 - gamma;
			location = Point::PhiToPoint(pA, sweeplineX, theta, phi);

			const float PI = 3.14159265359f;


			if (location.phi > PI) location.phi -= PI * 2;
			if (location.phi < -PI) location.phi += PI * 2;

			return true;
		}
	};

	struct CircleEvent
	{
		CircleEvent(BeachArc* i, BeachArc* j, BeachArc* k) : i(i), j(j), k(k)
		{
			auto p_ij = i->cell->position - j->cell->position;
			auto p_kj = k->cell->position - j->cell->position;

			auto dir = cross(p_ij, p_kj);

			center = Point(dir);
			theta = acos(center.position.z) + acos(glm::dot(center.position, i->cell->position));
		}

		CircleEvent(BeachArc* i, BeachArc* j, BeachArc* k, Point center, float theta) : i(i), j(j), k(k), center(center), theta(theta) { }

		BeachArc* i;
		BeachArc* j;
		BeachArc* k;

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
		Skiplist<BeachArc> beach;

		void Step(float maxDeltaX);

		bool Finished() { return siteEventQueue.empty(); }

		Planet* RetrievePlanet() const { return planet; };

	private:
		const float PI = 3.14159265359f;

		void GeneratePoints(int numPoints);
		Point GeneratePoint(int index, int numPoints);
		void InitialiseEvents();

		void HandleSiteEvent(Point* event);
		void HandleCircleEvent(CircleEvent* event);

		void CheckForValidCircleEvent(Point* i, Point* j, Point* k, float sweeplineX);


		void AddCircleEvent(Point* i, Point* j, Point* k, Point center, float theta);
		void RemoveCircleEvent(CircleEvent* event);
	};
}
