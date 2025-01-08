#pragma once

namespace Dymatic {

	typedef glm::vec2 DelaunayVertex;

	struct DelaunayTriangleResult
	{
		uint32_t V0, V1, V2;

		DelaunayTriangleResult(const uint32_t v0, const uint32_t v1, const uint32_t v2)
			: V0(v0), V1(v1), V2(v2) {}
	};

	struct DelaunayEdgeResult
	{
		uint32_t V0, V1;

		DelaunayEdgeResult(const uint32_t v0, const uint32_t v1)
			: V0(v0), V1(v1) {}

		bool operator==(const DelaunayEdgeResult& other) const
		{
			return V0 == other.V0 && V1 == other.V1;
		}
	};

	void DelaunaryTriangulate(const std::vector<DelaunayVertex>& vertices, std::vector<DelaunayTriangleResult>& resultTriangles);
	void DelaunaryTraceOutsideEdges(const std::vector<DelaunayTriangleResult>& triangles, std::vector<DelaunayEdgeResult>& resultEdges);

}