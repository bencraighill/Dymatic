#pragma once

#include "Dymatic/Renderer/Mesh.h"
#include "Dymatic/Core/Hash.h"

#include <set>
#include <queue>

namespace Dymatic {

	class LODGenerator
	{
	public:
		class CrossLink
		{
		public:
			CrossLink(int size);
			~CrossLink();

			inline int Size() const { return s; }
			bool Insert(int x, int y);
			bool Find(int x, int y);
			void Erase(int x, int y);

		public:
			struct Node
			{
				Node* lhs, * rhs, * ups, * dwn;
				int x;
				int y;
				
				Node(int x, int y) : x(x), y(y)
				{
					lhs = rhs = ups = dwn = nullptr;
				}
			};
			
			int s;
			Node** edgeX;
			Node** edgeY;
		};
		
		struct Edge
		{
			int v1, v2;
			
			Edge(int v1 = -1, int v2 = -1)
				: v1(v1), v2(v2)
			{}

			bool operator < (const Edge& e2) const
			{
				if (v1 < e2.v1)
					return true;
				else if (v1 == e2.v1)
					return v2 < e2.v2;
				else
					return false;
			}
		};

		class Face
		{
		public:
			Face(int v1, int v2, int v3) : v1(v1), v2(v2), v3(v3) {};
			~Face() {}
			
			Edge Against(int v) const;
			void Reverse();
			void Replace(int v, int vNew);
			
		public:
			int v1, v2, v3;
		};

		struct LODVertex
		{
			glm::vec3 Position;
			uint32_t SourceIndex;
		};
		
	public:
		LODGenerator(const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indicies);
		~LODGenerator();

		void Simplify(const size_t target, const double threashold);

		void UnpackResult(std::vector<MeshVertex>& vertices, std::vector<uint32_t>& indices);
			
	private:
		double EdgeLength(const Edge& e);
		
		std::pair<LODVertex, double> GetPosition(const Edge& e);
		std::pair<Edge, LODVertex> SelectEdge(double threshold);
		
		bool FaceReverse(const Edge& e, const glm::vec3& v1, const glm::vec3& v2);
		void AddToHeap(const Edge& e, double threshold);
		void UpdateNeighborEdge(int v, double threshold);

		void RemoveEdge(const Edge& e, const LODVertex& v, double threshold);
		void BuildHeap(double threshold);
		
	private:
		std::vector<LODVertex> m_Vertices;
		std::vector<MeshVertex> m_SourceVertices;
		std::unordered_map<uint32_t, uint32_t> m_SourceToUniqueVertexMap;
		double m_Threashold;

		size_t m_NumFaces, m_OldNumEdges, m_OldNumFaces, m_OldNumVertices;

		std::vector<bool> m_VertexRemoved;

		std::vector<Face> m_Faces;
		std::vector<std::unordered_set<int>> m_VertexFaceTable;

		CrossLink* m_Edges;

		std::priority_queue<std::pair<double, Edge>> m_Heap;
	};

}