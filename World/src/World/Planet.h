#pragma once
#include <vector>
#include <glm/vec3.hpp>

namespace World
{
	class Planet
	{
	public:

		struct HalfEdge
		{
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
			std::vector<HalfEdge> edges;
		};


		VoronoiCell CellAt(glm::vec3 location);


		std::vector<VoronoiCell> cells; // Sorted
		
		std::vector<glm::vec3> edgeVertices;
		std::vector<HalfEdge> halfEdges;
	};
}
