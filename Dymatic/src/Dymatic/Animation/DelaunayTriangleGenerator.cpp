#include "dypch.h"
#include "Dymatic/Animation/DelaunayTriangleGenerator.h"

#include "Dymatic/Core/Hash.h"

#include <unordered_map>
#include <unordered_set>

// Delaunay Triangulation Algorithm

namespace Dymatic {

	struct DelaunayEdge
	{
		DelaunayVertex V0, V1;

		DelaunayEdge(const DelaunayVertex& v0, const DelaunayVertex& v1)
			: V0(v0), V1(v1) {}

		bool operator==(const DelaunayEdge& other) const
		{
			return (V0 == other.V0) && (V1 == other.V1);
		}
		
		DelaunayEdge Inverse() const
		{
			return DelaunayEdge(V1, V0);
		}
	};

	struct DelaunayTriangle
	{
		DelaunayVertex V0, V1, V2;
		DelaunayVertex Center;
		float Radius;

		DelaunayTriangle(const DelaunayVertex& v0, const DelaunayVertex& v1, const DelaunayVertex& v2)
			: V0(v0), V1(v1), V2(v2)
		{
			CalculateCircumcircle();
		}

		void CalculateCircumcircle();
		bool InCircumcircle(const DelaunayVertex& vertex) const;
	};

	void DelaunayTriangle::CalculateCircumcircle()
	{
		float a = V1.x - V0.x;
		float b = V1.y - V0.y;
		float c = V2.x - V0.x;
		float d = V2.y - V0.y;

		float e = a * (V0.x + V1.x) + b * (V0.y + V1.y);
		float f = c * (V0.x + V2.x) + d * (V0.y + V2.y);

		float g = 2.0 * (a * (V2.y - V1.y) - b * (V2.x - V1.x));

		float dx, dy;

		if (glm::abs(g) < glm::epsilon<float>())
		{
			// Collinear - find extremes and use the midpoint
			const float minX = std::min({ V0.x, V1.x, V2.x });
			const float minY = std::min({ V0.y, V1.y, V2.y });
			const float maxX = std::max({ V0.x, V1.x, V2.x });
			const float maxY = std::max({ V0.y, V1.y, V2.y });

			Center = DelaunayVertex((minX + maxX) * 0.5f, (minY + maxY) * 0.5f);
			dx = Center.x - minX;
			dy = Center.y - minY;
		}
		else
		{
			const float cx = (d * e - b * f) / g;
			const float cy = (a * f - c * e) / g;

			Center = DelaunayVertex(cx, cy);
			dx = Center.x - V0.x;
			dy = Center.y - V0.y;
		}

		Radius = std::sqrt(dx * dx + dy * dy);
	}

	bool DelaunayTriangle::InCircumcircle(const DelaunayVertex& vertex) const
	{
		return glm::distance(Center, vertex) < Radius;
	}

	static DelaunayTriangle CreateBoundingTriangle(const std::vector<DelaunayVertex>& vertices)
	{
		glm::vec2 min = glm::vec2(std::numeric_limits<float>::max());
		glm::vec2 max = glm::vec2(std::numeric_limits<float>::lowest());

		for (const auto& vertex : vertices)
		{
			min = glm::min(min, vertex);
			max = glm::max(max, vertex);
		}

		const float dx = (max.x - min.x) * 10;
		const float dy = (max.y - min.y) * 10;

		DelaunayVertex stv0(min.x - dx, min.y - dy * 3);
		DelaunayVertex stv1(min.x - dx, max.y + dy);
		DelaunayVertex stv2(max.x + dx * 3, max.y + dy);

		return DelaunayTriangle(stv0, stv1, stv2);
	}

	static void UniqueEdges(std::vector<DelaunayEdge>& edges)
	{
		for (size_t i = 0; i < edges.size(); i++)
		{
			bool unique = true;

			for (size_t j = i + 1; j < edges.size(); j++)
			{
				if (edges[i] == edges[j] || edges[i].Inverse() == edges[j])
				{
					unique = false;
					edges.erase(edges.begin() + j);
					j--;
				}
			}

			if (!unique)
			{
				edges.erase(edges.begin() + i);
				i--;
			}
		}
	}

	static void AddVertex(const DelaunayVertex& vertex, std::vector<DelaunayTriangle>& triangles)
	{
		std::vector<DelaunayEdge> edges;

		// Remove triangles with circumcircles containing the vertex
		triangles.erase(std::remove_if(triangles.begin(), triangles.end(), [&vertex, &edges](const DelaunayTriangle& triangle)
		{
			if (triangle.InCircumcircle(vertex))
			{
				edges.emplace_back(triangle.V0, triangle.V1);
				edges.emplace_back(triangle.V1, triangle.V2);
				edges.emplace_back(triangle.V2, triangle.V0);
				return true;
			}

			return false;
		}), triangles.end());

		UniqueEdges(edges);

		// Create new triangles from the unique edges and the new vertex
		for (const auto& edge : edges)
			triangles.emplace_back(edge.V0, edge.V1, vertex);
	}

	void DelaunaryTriangulate(const std::vector<DelaunayVertex>& vertices, std::vector<DelaunayTriangleResult>& resultTriangles)
	{
		std::vector<DelaunayTriangle> triangles;

		// First, create 'supertriangle' bounding all vertices
		DelaunayTriangle supertriangle = CreateBoundingTriangle(vertices);
		triangles.emplace_back(supertriangle);

		// Next, begin the triangulation one vertex at a time
		for (const auto& vertex : vertices)
			AddVertex(vertex, triangles);

		// Remove triangles that share edges with 'supertriangle'
		triangles.erase(std::remove_if(triangles.begin(), triangles.end(), [&supertriangle](const DelaunayTriangle& triangle)
		{
			return (triangle.V0 == supertriangle.V0 || triangle.V0 == supertriangle.V1 || triangle.V0 == supertriangle.V2 ||
					triangle.V1 == supertriangle.V0 || triangle.V1 == supertriangle.V1 || triangle.V1 == supertriangle.V2 ||
					triangle.V2 == supertriangle.V0 || triangle.V2 == supertriangle.V1 || triangle.V2 == supertriangle.V2);
		}), triangles.end());

		// Convert the triangles internal format back to a vector of indices to return
		resultTriangles.clear();
		resultTriangles.reserve(triangles.size());

		std::unordered_map<DelaunayVertex, uint32_t> vertexIndexMap(vertices.size());
		for (uint32_t vertexIndex = 0; vertexIndex < vertices.size(); vertexIndex++)
			vertexIndexMap[vertices[vertexIndex]] = vertexIndex;

		for (const auto& triangle : triangles)
			resultTriangles.emplace_back(vertexIndexMap[triangle.V0], vertexIndexMap[triangle.V1], vertexIndexMap[triangle.V2]);
	}

	struct EdgeHash
	{
		std::size_t operator()(const DelaunayEdgeResult& edge) const
		{
			std::hash<uint32_t> hasher;
			std::size_t h1 = hasher(edge.V0);
			std::size_t h2 = hasher(edge.V1);

			return h1 ^ (h2 << 1);
		}
	};

	static DelaunayEdgeResult NormalizeEdge(const uint32_t v0, const uint32_t v1)
	{
		// Ensure edge pairs are always inserted with minimum and maximum in the same orientation
		// so hashing will compare correctly regardless of orientation.
		return DelaunayEdgeResult(std::min(v0, v1), std::max(v0, v1));
	}

	void DelaunaryTraceOutsideEdges(const std::vector<DelaunayTriangleResult>& triangles, std::vector<DelaunayEdgeResult>& resultEdges)
	{
		std::unordered_map<DelaunayEdgeResult, uint32_t, EdgeHash> includedEdgeSet(triangles.size() * 3);

		for (const auto& triangle : triangles)
		{
			includedEdgeSet[NormalizeEdge(triangle.V0, triangle.V1)]++;
			includedEdgeSet[NormalizeEdge(triangle.V1, triangle.V2)]++;
			includedEdgeSet[NormalizeEdge(triangle.V2, triangle.V0)]++;
		}

		// Filter out any internal edges (edges appearing more than once)
		resultEdges.clear();
		for (const auto& [edge, count] : includedEdgeSet)
		{
			if (count == 1)
				resultEdges.emplace_back(edge);
		}
	}

}