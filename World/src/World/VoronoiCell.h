#pragma once
#include <vector>

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

	class VoronoiCell
	{
		uint32_t index = 0;
		glm::vec3 cartesian;

		std::vector<uint32_t> edges;
		// These must be ordered such that for all i = [ 1, 2, 3 ... n - 2 ], j = i + 1
		// (i x j) . point > 0 or glm::dot ( glm::cross(i, j), point ) > 0

	public:
		VoronoiCell(glm::vec3 position)
		{
			this->cartesian = position;
		}

		uint32_t GetIndex() const { return index; }
		void SetIndex(uint32_t index) { this->index = index; }
		
		glm::vec3 GetCenter() const { return cartesian; }

		void AddEdge(uint32_t edgeId)
		{
			edges.emplace_back(edgeId);
		}
		
		std::vector<glm::vec3> GetFaceVertices(const std::vector<HalfEdge>& halfEdges, const std::vector<glm::vec3>& edgeVertices) const 
		{
			std::vector<glm::vec3> vertices;
			
			for (auto edge : edges)
			{
				const auto& halfEdge = halfEdges[edge];

				// the .9995f is a cheap hack to get around floating point precision & drawing straight lines over the surface of a sphere
				// if it is not shrunk slightly, there is z fighting between the edges / sites and the face
				vertices.emplace_back(edgeVertices[halfEdge.beginIndex] * .9995f);
				vertices.emplace_back(cartesian * .9995f );
				vertices.emplace_back(edgeVertices[halfEdge.endIndex] * .9995f);
			}

			return vertices;
		}
	};
	
}
