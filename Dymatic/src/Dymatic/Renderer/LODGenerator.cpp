#include "dypch.h"
#include "Dymatic/Renderer/LODGenerator.h"

// LOD generation code based on Quadratic Error Metric implementation from Mesh-- (mmm) by seanzw (https://github.com/seanzw/Mesh--)

#define EPS 1e-8

namespace Dymatic {

	LODGenerator::CrossLink::CrossLink(int size)
		: s(size)
	{
		edgeX = new Node * [size];
		edgeY = new Node * [size];
		for (int i = 0; i < size; i++)
			edgeX[i] = edgeY[i] = nullptr;
	}

	LODGenerator::CrossLink::~CrossLink()
	{
		for (int i = 0; i < s; i++)
		{
			Node* node = edgeX[i];
			while (node != nullptr)
			{
				Node* next = node->dwn;
				delete node;
				node = next;
			}
		}

		delete[] edgeX;
		delete[] edgeY;
	}

	bool LODGenerator::CrossLink::Insert(int x, int y)
	{
		Node* p = edgeX[x];
		Node* node = new Node(x, y);
		if (p == nullptr)
			edgeX[x] = node;
		else
		{
			while (p != nullptr)
			{
				if (p->y > y)
				{
					node->dwn = edgeX[x];
					edgeX[x]->ups = node;
					edgeX[x] = node;
					break;
				}
				else if (p->y == y)
				{
					delete node;
					return true;
				}
				else
				{
					if (p->dwn == nullptr)
					{
						p->dwn = node;
						node->ups = p;
						break;
					}
					else if (p->dwn->y > y)
					{
						node->dwn = p->dwn;
						node->ups = p;
						p->dwn->ups = node;
						p->dwn = node;
						break;
					}
					else
					{
						p = p->dwn;
					}
				}
			}
		}

		p = edgeY[y];
		if (p == nullptr)
			edgeY[y] = node;
		else
		{
			while (p != nullptr)
			{
				if (p->x > x)
				{
					node->rhs = edgeY[y];
					edgeY[y]->lhs = node;
					edgeY[y] = node;
					break;
				}
				else if (p->x == x)
				{
					return true;
				}
				else
				{
					if (p->rhs == nullptr)
					{
						p->rhs = node;
						node->lhs = p;
						break;
					}
					else if (p->rhs->x > x)
					{
						node->rhs = p->rhs;
						node->lhs = p;
						p->rhs->lhs = node;
						p->rhs = node;
						break;
					}
					else
					{
						p = p->rhs;
					}
				}
			}
		}

		return false;
	}

	bool LODGenerator::CrossLink::Find(int x, int y)
	{
		Node* p = edgeX[x];
		if (p == nullptr)
			return false;
		else
		{
			while (p != nullptr)
			{
				if (p->y > y)
				{
					return false;
				}
				else if (p->y == y)
				{
					return true;
				}
				else
				{
					if (p->dwn == nullptr)
					{
						return false;
					}
					else if (p->dwn->y > y)
					{
						return false;
					}
					else
					{
						p = p->dwn;
					}
				}
			}
		}

		return false;
	}

	void LODGenerator::CrossLink::Erase(int x, int y)
	{
		Node* p = edgeX[x];

		while (p != nullptr)
		{
			if (p->y > y)
			{
				break;
			}
			else if (p->y == y)
			{

				if (p->lhs != nullptr)
				{
					p->lhs->rhs = p->rhs;
				}
				else
				{
					DY_CORE_ASSERT(p == edgeY[y]);
					edgeY[y] = p->rhs;
				}

				if (p->rhs != nullptr)
				{
					p->rhs->lhs = p->lhs;
				}

				if (p->ups != nullptr)
				{
					p->ups->dwn = p->dwn;
				}
				else
				{
					DY_CORE_ASSERT(p == edgeX[x]);
					edgeX[x] = p->dwn;
				}

				if (p->dwn != nullptr)
				{
					p->dwn->ups = p->ups;
				}

				delete p;
				break;
			}
			else
			{
				if (p->dwn == nullptr)
				{
					break;
				}
				else if (p->dwn->y > y)
				{
					break;
				}
				else
				{
					p = p->dwn;
				}
			}
		}
	}

	LODGenerator::Edge LODGenerator::Face::Against(int v) const
	{
		DY_CORE_ASSERT(v == v1 || v == v2 || v == v3);

		if (v == v1)
			return Edge(v2, v3);
		else if (v == v2)
			return Edge(v3, v1);
		else
			return Edge(v1, v2);
	}

	void LODGenerator::Face::Reverse()
	{
		std::swap(v1, v2);
	}

	void LODGenerator::Face::Replace(int v, int vNew)
	{
		DY_CORE_ASSERT(v == v1 || v == v2 || v == v3);

		if (v == v1)
			v1 = vNew;
		else if (v == v2)
			v2 = vNew;
		else
			v3 = vNew;
	}

	LODGenerator::LODGenerator(const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices)
		: m_SourceVertices(vertices)
	{
		// Process vertices (merge vertices with identical positions, regardless of attributes)
		std::unordered_map<glm::vec3, uint32_t> uniqueVertexMap;
		for (size_t sourceVertexIndex = 0; sourceVertexIndex < vertices.size(); sourceVertexIndex++)
		{
			const auto& sourceVertex = m_SourceVertices[sourceVertexIndex];

			if (uniqueVertexMap.find(sourceVertex.Position) == uniqueVertexMap.end())
			{
				uniqueVertexMap[sourceVertex.Position] = m_Vertices.size();
				m_Vertices.push_back({ sourceVertex.Position });
			}

			const uint32_t uniqueIndex = uniqueVertexMap.at(sourceVertex.Position);
			m_SourceToUniqueVertexMap[sourceVertexIndex] = uniqueIndex;
		}

		// Initialize Structures
		m_VertexFaceTable = std::vector<std::unordered_set<int>>(m_Vertices.size());
		m_VertexRemoved = std::vector<bool>(m_Vertices.size(), false);
		m_Edges = new CrossLink(m_Vertices.size());

		// Process indices to create faces and edges
		for (size_t i = 0; i < indices.size(); i += 3)
		{
			int id[3] = {
				m_SourceToUniqueVertexMap[indices[i + 0]],
				m_SourceToUniqueVertexMap[indices[i + 1]],
				m_SourceToUniqueVertexMap[indices[i + 2]]
			};

			m_Faces.emplace_back(id[0], id[1], id[2]);
			m_VertexFaceTable[id[0]].insert((int)m_Faces.size() - 1);
			m_VertexFaceTable[id[1]].insert((int)m_Faces.size() - 1);
			m_VertexFaceTable[id[2]].insert((int)m_Faces.size() - 1);

			std::sort(id, id + 3);
			DY_CORE_ASSERT(0 <= id[0] && id[0] < id[1] && id[1] < id[2] && id[2] < m_Vertices.size());
			m_Edges->Insert(id[0], id[1]);
			m_Edges->Insert(id[1], id[2]);
			m_Edges->Insert(id[0], id[2]);
		}

		m_NumFaces = m_Faces.size();
		m_OldNumEdges = m_Edges->Size();
		m_OldNumFaces = m_Faces.size();
		m_OldNumVertices = m_Vertices.size();
	}

	LODGenerator::~LODGenerator()
	{
		delete m_Edges;
	}

	void LODGenerator::Simplify(const size_t target, const double threashold)
	{
		m_Threashold = threashold;

		DY_CORE_TRACE("Simplifying mesh with threashold: {}", m_Threashold);

		// Build the heap.
		BuildHeap(m_Threashold);

		while (m_NumFaces > target)
		{
			//  Select an edge to collapse.
			auto e = SelectEdge(m_Threashold);
			if (e.first.v1 != -1)
				RemoveEdge(e.first, e.second, m_Threashold);
			else
			{
				// Failed getting an edge to deleted.
				// Double the threshold.
				m_Threashold *= 2.0;
				DY_CORE_WARN("Not enough edges, doubling the threshold!");
				DY_CORE_TRACE("    New Threashold: {}", m_Threashold);
				BuildHeap(m_Threashold);
			}
		}
	}

	void LODGenerator::UnpackResult(std::vector<MeshVertex>& vertices, std::vector<uint32_t>& indices)
	{
		// Ensure the output vectors are empty
		vertices.clear();
		indices.clear();

		// Map old vertex indices to new ones
		std::unordered_map<int, int> vertexMap;
		int newIndex = 0;

		// Track the valid vertices and assign new indices
		for (size_t sourceVertexIndex = 0; sourceVertexIndex < m_SourceVertices.size(); sourceVertexIndex++)
		{
			const auto& sourceVertex = m_SourceVertices[sourceVertexIndex];

			if (m_SourceToUniqueVertexMap.find(sourceVertexIndex) == m_SourceToUniqueVertexMap.end())
			{
				DY_CORE_ASSERT(false);
				continue;
			}

			uint32_t lodVertexIndex = m_SourceToUniqueVertexMap[sourceVertexIndex];

			if (m_VertexRemoved[lodVertexIndex])
				continue;

			if (vertexMap.find(lodVertexIndex) != vertexMap.end())
				continue;

			vertexMap[lodVertexIndex] = newIndex++;

			const auto& lodVertex = m_Vertices[lodVertexIndex];

			// Push the new vertex with the modified position and original properties others
			MeshVertex newVertex = sourceVertex;
			newVertex.Position = lodVertex.Position;
			vertices.push_back(newVertex);
		}

		// Update the faces with new vertex indices
		for (const auto& face : m_Faces)
		{
			if (!m_VertexRemoved[face.v1] && !m_VertexRemoved[face.v2] && !m_VertexRemoved[face.v3])
			{
				indices.push_back(vertexMap.at(face.v1));
				indices.push_back(vertexMap.at(face.v2));
				indices.push_back(vertexMap.at(face.v3));
			}
		}
	}

	double LODGenerator::EdgeLength(const Edge& e)
	{
		//return norm(verts[e.v1] - verts[e.v2]);
		return glm::length(m_Vertices[e.v1].Position - m_Vertices[e.v2].Position);
	}

	std::pair<LODGenerator::LODVertex, double> LODGenerator::GetPosition(const Edge& e)
	{
		// Get the Q matrix for all the faces.
		glm::mat4 q(0.0);

		auto calculate = [&](int vId) -> void
			{
				for (const auto& fId : m_VertexFaceTable[vId])
				{
					Edge edge = m_Faces[fId].Against(vId);
					int v1 = edge.v1, v2 = edge.v2;
					auto n = glm::cross(m_Vertices[v1].Position - m_Vertices[vId].Position, m_Vertices[v2].Position - m_Vertices[vId].Position);
					n = glm::normalize(n);
					glm::vec4 plane(n, -glm::dot(m_Vertices[vId].Position, n));
					q += glm::outerProduct(plane, plane);
				}
			};

		calculate(e.v1);
		calculate(e.v2);

		// Blend vertex properties
		LODVertex vertex;
		vertex.Position = (m_Vertices[e.v1].Position + m_Vertices[e.v2].Position) * 0.5f;
		//vertex.Normal = (m_Vertices[e.v1].Normal + m_Vertices[e.v2].Normal) * 0.5f;
		//vertex.Tangent = (m_Vertices[e.v1].Tangent + m_Vertices[e.v2].Tangent) * 0.5f;
		//vertex.Bitangent = (m_Vertices[e.v1].Bitangent + m_Vertices[e.v2].Bitangent) * 0.5f;
		//vertex.TexCoords = (m_Vertices[e.v1].TexCoords + m_Vertices[e.v2].TexCoords) * 0.5f;

		// TODO: Blend bones vertex is affected by and the weights (difficult to do as weights are for the corresponding bone and limited by MAX_INFLUENCE)

		// Change this to vec4 and calculate the cost.
		glm::vec4 v4(vertex.Position, 1.0f);
		double cost = glm::dot(v4, q * v4);
		//DY_CORE_ASSERT(cost > -EPS);

		return std::make_pair(vertex, cost);
	}

	std::pair<LODGenerator::Edge, LODGenerator::LODVertex> LODGenerator::SelectEdge(double threshold)
	{
		Edge idx(-1, -1);
		LODVertex pos;
		std::pair<double, Edge> tmp;

		while (!m_Heap.empty())
		{
			tmp = m_Heap.top();
			m_Heap.pop();

			// This edge is already deleted.
			if (!m_Edges->Find(tmp.second.v1, tmp.second.v2))
				continue;

			// The verts of the edge has been deleted.
			if (m_VertexRemoved[tmp.second.v1] || m_VertexRemoved[tmp.second.v2])
				continue;

			// The edge is too long.
			if (EdgeLength(tmp.second) >= threshold)
				continue;

			// Again we get the split position.
			auto act = GetPosition(tmp.second);

			// Compare the new cost with the original one.
			if (fabs(act.second + tmp.first) > EPS)
				continue;

			// This is indeed a valid edge.
			idx = tmp.second;
			pos = act.first;

			break;
		}

		return std::make_pair(idx, pos);
	}

	bool LODGenerator::FaceReverse(const Edge& e, const glm::vec3& v1, const glm::vec3& v2)
	{
		const glm::vec3& x = m_Vertices[e.v1].Position;
		const glm::vec3& y = m_Vertices[e.v2].Position;

		return glm::dot(glm::cross(x - v1, y - v1), glm::cross(x - v2, y - v2)) < 0.0;
	}

	void LODGenerator::AddToHeap(const Edge& e, double threshold)
	{
		if (EdgeLength(e) > threshold)
			return;

		// Get the split position.
		const auto pos = GetPosition(e);

		m_Heap.emplace(-pos.second, e);
	}

	void LODGenerator::UpdateNeighborEdge(int v, double threshold)
	{
		std::unordered_set<int> neighbor;
		for (const auto& fId : m_VertexFaceTable[v])
		{
			Edge edge = m_Faces[fId].Against(v);
			neighbor.insert(edge.v1);
			neighbor.insert(edge.v2);
		}

		for (auto x : neighbor)
			AddToHeap(Edge(glm::min(x, v), glm::max(x, v)), threshold);
	}

	void LODGenerator::RemoveEdge(const Edge& e, const LODVertex& v, double threshold)
	{
		// First check if any face of v1 will be reversed after this removal.
		for (const auto& fId : m_VertexFaceTable[e.v1])
		{
			Edge edge = m_Faces[fId].Against(e.v1);

			// If this is the face we will remove, simply ignore it.
			if (edge.v1 == e.v2 || edge.v2 == e.v2)
				continue;

			bool reverse = FaceReverse(edge, m_Vertices[e.v1].Position, v.Position);
			if (!reverse)
				continue;

			// The face will be reversed after we replacing v1 with v.
			// Fix the orientation of the faces at face[f.v2] and face[f.v1].
			DY_CORE_ASSERT(m_VertexFaceTable[edge.v2].find(fId) != m_VertexFaceTable[edge.v2].end());
			DY_CORE_ASSERT(m_VertexFaceTable[edge.v1].find(fId) != m_VertexFaceTable[edge.v1].end());
			m_Faces[fId].Reverse();
		}

		for (const auto& fId : m_VertexFaceTable[e.v2])
		{
			/* Check if the face will be reverse. */
			Edge edge = m_Faces[fId].Against(e.v2);

			bool reverse = FaceReverse(edge, m_Vertices[e.v2].Position, v.Position);
			DY_CORE_ASSERT(m_VertexFaceTable[edge.v2].find(fId) != m_VertexFaceTable[edge.v2].end());
			DY_CORE_ASSERT(m_VertexFaceTable[edge.v1].find(fId) != m_VertexFaceTable[edge.v1].end());

			if (edge.v1 != e.v1 && edge.v2 != e.v1)
			{
				// This isn't the face we will collapse.
				m_Faces[fId].Replace(e.v2, e.v1);
				m_VertexFaceTable[e.v1].insert(fId);
				if (reverse)
					m_Faces[fId].Reverse();
			}
			else
			{
				m_VertexFaceTable[edge.v1].erase(fId);
				m_VertexFaceTable[edge.v2].erase(fId);
				m_NumFaces--;
			}

			// Update the edge.
			if (m_Edges->Find(glm::min(e.v2, edge.v1), glm::max(e.v2, edge.v1)))
				m_Edges->Erase(glm::min(e.v2, edge.v1), glm::max(e.v2, edge.v1));

			if (m_Edges->Find(glm::min(e.v2, edge.v2), glm::max(e.v2, edge.v2)))
				m_Edges->Erase(glm::min(e.v2, edge.v2), glm::max(e.v2, edge.v2));

			if (edge.v1 != e.v1 && edge.v2 != e.v1)
			{
				m_Edges->Insert(glm::min(e.v1, edge.v1), glm::max(e.v1, edge.v1));
				m_Edges->Insert(glm::min(e.v1, edge.v2), glm::max(e.v1, edge.v2));
			}
		}

		m_Edges->Erase(e.v1, e.v2);
		m_Vertices[e.v1] = v;
		m_VertexRemoved[e.v2] = true;
		m_VertexFaceTable[e.v2].clear();

		std::unordered_set<int> neighbor; // All the neighboring vert.
		for (const auto& fId : m_VertexFaceTable[e.v1])
		{
			Edge edge = m_Faces[fId].Against(e.v1);

			neighbor.insert(edge.v1);
			neighbor.insert(edge.v2);
		}

		for (const auto& nb : neighbor)
			UpdateNeighborEdge(nb, threshold);
	}

	void LODGenerator::BuildHeap(double threshold)
	{
		// First clear the heap.
		//while (!m_Heap.empty())
		//	m_Heap.pop();
		m_Heap = {};

		for (int i = 0; i < m_Edges->Size(); ++i)
		{
			CrossLink::Node* node = m_Edges->edgeX[i];

			while (node != nullptr)
			{
				AddToHeap(Edge(node->x, node->y), threshold);
				node = node->dwn;
			}
		}
	}

}