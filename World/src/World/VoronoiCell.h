#pragma once
#include <vector>
#include <numbers>

namespace World
{
	struct HalfEdge
	{
		bool finished;
		
		uint32_t beginIndex; // index into an array of vertices 
		uint32_t endIndex;

		uint32_t index; // indexes into an array of half edges (index points to itself)
		uint32_t nextIndex;
		uint32_t previousIndex;
	};

	class VoronoiCell
	{
		uint32_t index = 0;
		glm::vec3 cartesian;

		std::vector<uint32_t> edges;
		std::vector<uint32_t> delEdges;
		// These must be ordered such that for all i = [ 1, 2, 3 ... n - 2 ], j = i + 1
		// (i x j) . point > 0 or glm::dot ( glm::cross(i, j), point ) > 0

	public:
		explicit VoronoiCell(glm::vec3 position)
			: cartesian(position)
		{

		}

		uint32_t GetIndex() const { return index; }
		void SetIndex(uint32_t index) { this->index = index; }
		
		glm::vec3 GetCenter() const { return cartesian; }

		void AddEdge(uint32_t edgeId)
		{
			edges.emplace_back(edgeId);
		}

		void AddDelEdge(uint32_t edgeId)
		{
			this->delEdges.emplace_back(edgeId);
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

		// Based on https://stackoverflow.com/a/66176425
		std::vector<uint32_t> GetSortedDelEdges(const std::vector<HalfEdge>& delEdges, const std::vector<VoronoiCell>& cells)
		{
			// We consider the first integer to be the index into the array of edges
			// and the second to be the angle from the centroid of the cell to
			// the edges outgoing vertex
			std::vector< std::pair<int, double>> dists(this->delEdges.size() - 1);

			// The center point of our triangulation must be the voronoi point
			const auto& centroid = cartesian;
			const auto& initialEdge = delEdges[this->delEdges[0]];

			// the edge.BeginIndex == idx ? other : idx returns us the *outgoing* vertex of the edge
			const auto& initialPoint = cells[initialEdge.beginIndex == index ? initialEdge.endIndex : initialEdge.beginIndex].GetCenter();

			// We use the first point as an abitrary 'basis' to compare other cells to
			const auto basis = cross(initialPoint, centroid);
			
			for(int i = 1; i < this->delEdges.size(); i++)
			{
				const auto adjustedIndex = i - 1;
				const auto& edge = delEdges[this->delEdges[i]];
				const auto& point = cells[edge.beginIndex == index ? edge.endIndex : edge.beginIndex].GetCenter();

				// The actual index which points into the original array is not adjusted.
				dists[adjustedIndex] = {i, dot(basis, cross(centroid, point)) };

				// Check if it meets the `criteria (i x j) . point < 0`
				if(dot( cross(centroid - initialPoint, point - centroid), point ) >= 0)
				{
					dists[adjustedIndex].second = 2 * std::numbers::pi - dists[adjustedIndex].second;
				}
			}

			std::sort(dists.begin(), dists.end(), [](const auto& l, const auto& r)
			{
				return l.second < r.second;
			});

			std::vector<uint32_t> sortedEdges;

			// Add anchor
			sortedEdges.emplace_back(this->delEdges[0]);

			for(auto& [idx, _] : dists)
			{
				sortedEdges.emplace_back(this->delEdges[idx]);
			}

			this->delEdges = std::move(sortedEdges);
			
			return this->delEdges;
		}
	};
	
}
