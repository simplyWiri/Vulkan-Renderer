#pragma once
#include <vector>
#include <glm/vec3.hpp>

#include "VoronoiCell.h"

namespace World
{

	class Planet
	{
	public:
		std::vector<VoronoiCell> cells; 

		std::vector<glm::vec3> edgeVertices;
		std::vector<HalfEdge> halfEdges;

		std::vector<HalfEdge> delanuayEdges;
	};
}
