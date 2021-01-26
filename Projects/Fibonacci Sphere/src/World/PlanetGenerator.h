#pragma once
#include <vector>
#include <vulkan_core.h>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include "Skiplist.h"


namespace Renderer {
	struct VertexAttributes;
	struct DescriptorSetKey;
	struct GraphContext;
}

namespace Renderer::Memory
{
		class Allocator;
		class Buffer;
}

struct Point
{
	Point() : theta(0), phi(0) { updatePosition(); }
	Point(float theta, float phi)
		: theta(theta), phi(phi) { updatePosition(); }
	
	Point(glm::vec3 dir)
	{
		float radius = glm::length(dir);

		theta = acos(glm::clamp(dir.z / radius, -1.0f, 1.0f));
		phi = glm::atan(dir.y, dir.x);
		position = dir / radius;
	}

	float sphericalDistance(const Point& other) const
    {
        auto dot = glm::dot(position, other.position);
        return acos(glm::clamp(dot, -1.0f, 1.0f));
    }

    bool operator < (const Point& other)
    {
        return (theta < other.theta) || (theta == other.theta && phi < other.phi);
    }

	void updatePosition()
	{
		position = glm::vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
	}

	
	static Point PhiToPoint(float phi, float x, float theta, float otherPhi)
    {
        if (theta >= x)
        {
            return Point(x, phi);      // could be any point on the line segment
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

struct Cell
{
	Cell(int i, Point p) : index(i), point(p) { }
	
	Point point;
	int index;

	bool operator < (const Cell& other) { return point < other.point; }
};

struct SphericalLine
{
	SphericalLine() : direction(0,0,1), x(0) { };
	SphericalLine(glm::vec3 d, float x) : direction(d), x(x) { };

	glm::vec3 direction;
	float x;
};

struct CircleEvent;

struct BeachArc
{
	BeachArc(Cell* cell) : cell(cell) { }
	
	Cell* cell;
	CircleEvent* circleEvent;

	bool operator < (const BeachArc& other) const { return cell->point.phi < other.cell->point.phi; }

	// returns true if the two arcs intersect, location will be a ref to the location at which the intersection occurs
	bool Intersect(const BeachArc& other, float sweeplineX, Point& location)
	{
		float theta = cell->point.theta;
		float phi = cell->point.phi;

		float otherTheta = cell->point.theta;
		float otherPhi = cell->point.phi;

		if(theta >= sweeplineX) // If we are further 'across' the sphere than the sweepline
		{
			if(otherTheta >= sweeplineX) return false;
			
			location = Point::PhiToPoint(phi, sweeplineX, otherTheta, otherPhi);
			return true;
		}

		if(otherTheta >= sweeplineX)
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

		auto a = ( (cosSweeplineX - cosOtherTheta) * sinTheta * cosPhi ) - ( (cosSweeplineX - cosTheta) * sinotherTheta * cosOtherPhi );
		auto b = ( (cosSweeplineX - cosOtherTheta) * sinTheta * sinPhi ) - ( (cosSweeplineX - cosTheta) * sinotherTheta * sinOtherPhi );
		auto e = (cosTheta - cosOtherTheta) * sinSweeplineX;

		auto length = glm::sqrt(a*a + b*b);
		
		if(abs(a) > length || abs(b) > length) return false;

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
	CircleEvent(BeachArc* i, BeachArc* j, BeachArc* k)
		: i(i), j(j), k(k)
	{
		auto p_ij = i->cell->point.position - j->cell->point.position;
		auto p_kj = k->cell->point.position - j->cell->point.position;

		auto dir = glm::cross(p_ij, p_kj);

		center = Point(dir);
		auto radius = acos(glm::dot(center.position, i->cell->point.position));
		theta = acos(center.position.z) + radius;
	}

	CircleEvent(BeachArc* i, BeachArc* j, BeachArc* k, Point center, float theta)
		: i(i), j(j), k(k), center(center), theta(theta) { }

	BeachArc* i;
	BeachArc* j;
	BeachArc* k;

	Point center; // Circumcenter
	float theta; // Lowest point on circle

	bool operator < (const CircleEvent& other) const { return theta < other.theta; }
};


class PlanetGenerator
{
public:
	
	explicit PlanetGenerator(Renderer::Memory::Allocator* alloc);
	~PlanetGenerator();
	
	SphericalLine scanline;
	int steps = 0;
	std::vector<glm::vec3> points;
	
	std::vector<Cell*> cells;
	
	std::vector<Cell*> siteEventQueue; // We are reaching a new point we haven't encountered yet
	std::vector<CircleEvent*> circleEventQueue; // One of the parabolas we have drawn for an existing (processed) point has disappeared
	Skiplist<BeachArc> beach;
	

	Renderer::Memory::Buffer* buf;
	Renderer::Memory::Buffer* scanlineBuf;


	
	void DrawPlanet(VkCommandBuffer buf);
	void Step(float maxDeltaX);

	bool Finished() { return siteEventQueue.empty(); }

private:

	void GeneratePoints(int numPoints);
	std::pair<float, float> GeneratePoint(int index, int numPoints);
	void InitialiseEvents(const std::vector<glm::vec3>& points);

	void HandleSiteEvent(Cell* event);
	void HandleCircleEvent(CircleEvent* event);

	void CheckForValidCircleEvent(Cell* i, Cell* j, Cell* k, float sweeplineX);
	void AddCircleEvent(Cell* i, Cell* j, Cell* k, Point center, float theta);

};

