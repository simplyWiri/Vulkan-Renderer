#pragma once
#include <vector>
#include <glm/vec3.hpp>

namespace World
{
	struct HalfEdge
	{
		bool finished;
		
		uint32_t beginIndex; // index into an array of vertices 
		uint32_t endIndex;

		uint32_t index; // indexes into an array of half edges (index points to itself)
		uint32_t pairIndex;
		uint32_t nextIndex;
		uint32_t previousIndex;

	};

	struct VoronoiCell
	{
		glm::vec3 point;
		std::vector<uint32_t> edges;
		// These must be ordered following the rule i = [ 1, 2, 3 ... n - 1 ], j = i + 1
		// (i x j) . point > 0
		// glm::dot ( glm::cross(i, j), point ) > 0
	};

	class Planet
	{
	public:
		VoronoiCell CellAt(glm::vec3 location);

		std::vector<VoronoiCell> cells; // Sorted

		std::vector<glm::vec3> edgeVertices;
		std::vector<HalfEdge> halfEdges;

		std::vector<glm::vec3> delanuayCells;
		std::vector<HalfEdge> delanuayEdges;
	};
}
