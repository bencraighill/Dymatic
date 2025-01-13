#include "dypch.h"
#include "Dymatic/Scene/Components.h"

namespace Dymatic {

	ParticleSystemComponent::ParticleSystemComponent(const ParticleSystemComponent& other)
	{
		if (other.Player)
			SetParticleSystem(other.Player->GetParticleSystem(), other.Material);
		else
			Material = other.Material;
	}

	void ParticleSystemComponent::SetParticleSystem(const Ref<ParticleSystem> particleSystem, const Ref<MaterialAsset> material /*= nullptr*/)
	{
		Player = particleSystem ? ParticleSystemPlayer::Create(particleSystem) : nullptr;
		Material = material ? material : (particleSystem ? AssetManager::GetAsset<MaterialAsset>(particleSystem->GetMaterialHandle()) : nullptr);
	}

	StaticMeshComponent::StaticMeshComponent(const StaticMeshComponent& other)
	{
		m_Model = other.m_Model;
		m_Materials = other.m_Materials;

		if (other.m_AnimationGraphPlayer)
			m_AnimationGraphPlayer = AnimationGraphPlayer::Create(other.m_Model, other.m_AnimationGraphPlayer->GetAnimationGraph());
	}

	void StaticMeshComponent::SetModel(Ref<Model> model)
	{
		m_Model = model;
		m_Materials.clear();
		m_AnimationGraphPlayer = nullptr;

		if (model && model->IsLoaded())
			m_Materials.resize(m_Model->GetMeshes().size());
	}

	void StaticMeshComponent::SetAnimationGraph(Ref<AnimationGraph> animationGraph)
	{
		if (!m_Model)
			return;

		m_AnimationGraphPlayer = nullptr;

		if (animationGraph && animationGraph->IsValid())
			m_AnimationGraphPlayer = AnimationGraphPlayer::Create(m_Model, animationGraph);
	}

	void StaticMeshComponent::Update(Timestep ts)
	{
		if (m_AnimationGraphPlayer)
			m_AnimationGraphPlayer->OnUpdate(ts);
	}

	void AudioComponent::SetStartPosition(const uint32_t startPosition)
	{
		StartPosition = startPosition > AudioSound->GetPlayLength() ? 0 : startPosition;
	}

	SplineComponent::SplineComponent()
	{
		Points.emplace_back(glm::vec3(0.0f, 0.0f, 0.0f));
		Points.emplace_back(glm::vec3(1.0f, 0.0f, 0.0f));
		Invalidate();
	}

	glm::vec3 SplineComponent::Sample(float t) const
	{
		if (Points.empty())
			return glm::vec3(0.0f);

		size_t numPoints = Points.size();

		// Map t to a segment index and a local segment t
		t = glm::clamp(t, 0.0f, 1.0f);
		float scaledT = t * (numPoints - 1);
		int segmentIndex = glm::floor(scaledT);
		float localT = scaledT - segmentIndex;

		segmentIndex = glm::clamp(segmentIndex, 0, (int)numPoints - 2);
		return SampleSegment(Points[segmentIndex], Points[segmentIndex + 1], localT);
	}

	glm::vec3 SplineComponent::SampleDistance(float distance) const
	{
		if (SegmentLengths.empty())
			return glm::vec3(0.0f);

		if (distance <= 0.0f)
			return Points.front().Position;

		if (distance >= TotalLength)
			return Points.back().Position;

		// Traverse the segments to find the correct one
		float accumulatedLength = 0.0f;
		for (size_t i = 0; i < SegmentLengths.size(); i++)
		{
			const float segmentLength = SegmentLengths[i];
			if (accumulatedLength + segmentLength >= distance)
			{
				float localDistance = distance - accumulatedLength;
				float localT = localDistance / segmentLength;

				return SampleSegment(Points[i], Points[i + 1], localT);
			}

			accumulatedLength += segmentLength;
		}

		// Return the last point if distance exceeds total length
		return Points.back().Position;
	}

	void SplineComponent::AddPoint()
	{
		Points.push_back(Points.back());
		Invalidate();
	}

	void SplineComponent::RemovePoint(uint32_t index)
	{
		Points.erase(Points.begin() + index);
		Invalidate();
	}

	void SplineComponent::DuplicatePoint(uint32_t index)
	{
		Points.insert(Points.begin() + index + 1, Points[index]);
		Invalidate();
	}

	void SplineComponent::SetPoints(const std::vector<SplinePoint>& points)
	{
		Points = points;
		Invalidate();
	}

	void SplineComponent::Invalidate()
	{
		TotalLength = 0.0f;

		if (Points.empty())
			SegmentLengths.clear();

		const size_t segmentCount = Points.size() - 1;
		SegmentLengths.resize(segmentCount);

		for (size_t segmentIndex = 0; segmentIndex < segmentCount; segmentIndex++)
		{
			const auto& startPoint = Points[segmentIndex + 0];
			const auto& endPoint = Points[segmentIndex + 1];

			float segmentLength = 0.0f;

			switch (startPoint.Type)
			{
			case SplineComponent::SplineType::Curve:
			{
				static constexpr uint32_t c_SplineSampleCount = 100;

				glm::vec3 previousSample = startPoint.Position;
				for (uint32_t sampleIndex = 1; sampleIndex <= c_SplineSampleCount; sampleIndex++)
				{
					const glm::vec3 newSample = SampleSegment(startPoint, endPoint, (float)sampleIndex / (float)c_SplineSampleCount);
					segmentLength += glm::distance(previousSample, newSample);
					previousSample = newSample;
				}

				break;
			}
			case SplineComponent::SplineType::Linear:
			{
				// Linear interpolation
				segmentLength = glm::distance(startPoint.Position, endPoint.Position);
				break;
			}
			}

			SegmentLengths[segmentIndex] = segmentLength;
			TotalLength += segmentLength;
		}
	}

	glm::vec3 SplineComponent::SampleSegment(const SplinePoint& startPoint, const SplinePoint& endPoint, float localT) const
	{
		switch (startPoint.Type)
		{
		default:
		case SplineComponent::SplineType::Curve:
		{
			// Hermite (Catmull-Rom) curve interpolation
			glm::vec3 p0 = startPoint.Position;
			glm::vec3 p1 = endPoint.Position;
			glm::vec3 m0 = startPoint.Tangent;
			glm::vec3 m1 = endPoint.Tangent;

			// Hermite spline formula
			float t2 = localT * localT;
			float t3 = t2 * localT;

			glm::vec3 result =
				(2.0f * t3 - 3.0f * t2 + 1.0f) * p0 +
				(t3 - 2.0f * t2 + localT) * m0 +
				(-2.0f * t3 + 3.0f * t2) * p1 +
				(t3 - t2) * m1;

			return result;
		}
		case SplineComponent::SplineType::Linear:
		{
			// Linear interpolation
			return glm::mix(startPoint.Position, endPoint.Position, localT);
		}
		case SplineComponent::SplineType::Constant:
		{
			// Constant interpolation (snap to the start of the segment)
			return startPoint.Position;
		}
		}
	}

	void FieldComponent::SetType(FieldType type)
	{
		Type = type;

		// Setup Defaults for Type
		if (type == FieldType::Directional)
			Force = glm::vec3(0.0f);
		else if (type == FieldType::Radial)
		{
			Magnitude = 1.0f;
			Radius = 1.0f;
			Falloff = 0.0f;
		}
		else if (type == FieldType::Buoyancy)
		{
			Buoyancy = 1.1f;
			LinearDrag = 0.3f;
			AngularDrag = 0.05f;
			FluidVelocity = glm::vec3(0.0f);
		}
	}

	LandscapeComponent::LandscapeComponent()
	{
		Allocate();
	}

	void LandscapeComponent::Allocate()
	{
		Data = nullptr;

		if (Resolution.x == 0 || Resolution.y == 0)
			return;

		Data = ScopedBuffer::Create(Resolution.x * Resolution.y * sizeof(float));
		Data->ZeroInitialize();

		Build();
	}

	void LandscapeComponent::Build()
	{
		LandscapeMesh = nullptr;

		if (Resolution.x == 0 || Resolution.y == 0)
			return;

		std::vector<MeshVertex> vertices;
		vertices.reserve(Resolution.x * Resolution.y);

		for (uint32_t y = 0; y < Resolution.y; y++)
		{
			for (uint32_t x = 0; x < Resolution.x; x++)
			{
				const glm::vec2 texCoords = glm::vec2((float)x / (float)Resolution.x, (float)y / (float)Resolution.y);
				const float height = Data->Get<float>(x + Resolution.x * y);
				const glm::vec3 position = glm::vec3(texCoords.x, height, texCoords.y);;

				vertices.emplace_back(x + Resolution.x * y, position, glm::vec3(0.0f), texCoords, glm::vec3(0.0f), glm::vec3(0.0f), glm::vec4(0.0f));
			}
		}

		std::vector<uint32_t> indices;
		indices.reserve(Resolution.x * (Resolution.y - 1) * 2);

		for (uint32_t y = 0; y < Resolution.y - 1; y++)
		{
			for (uint32_t x = 0; x < Resolution.x - 1; x++)
			{
				const uint32_t topLeft = x + Resolution.x * y;
				const uint32_t topRight = (x + 1) + Resolution.x * y;
				const uint32_t bottomLeft = x + Resolution.x * (y + 1);
				const uint32_t bottomRight = (x + 1) + Resolution.x * (y + 1);

				// Calculate indices
				indices.push_back(topLeft);
				indices.push_back(bottomLeft);
				indices.push_back(bottomRight);

				indices.push_back(topLeft);
				indices.push_back(bottomRight);
				indices.push_back(topRight);

				// Fetch attributes
				const glm::vec3& posTL = vertices[topLeft].Position;
				const glm::vec3& posTR = vertices[topRight].Position;
				const glm::vec3& posBL = vertices[bottomLeft].Position;
				const glm::vec3& posBR = vertices[bottomRight].Position;

				const glm::vec2& uvTL = vertices[topLeft].TexCoords;
				const glm::vec2& uvTR = vertices[topRight].TexCoords;
				const glm::vec2& uvBL = vertices[bottomLeft].TexCoords;
				const glm::vec2& uvBR = vertices[bottomRight].TexCoords;

				// Accumulate normal, tangent and bitangent properties for all vertices
				glm::vec3 edge1, edge2;
				glm::vec2 deltaUV1, deltaUV2;
				float f;

				// Triangle 1
				edge1 = posBL - posTL;
				edge2 = posBR - posTL;
				deltaUV1 = uvBL - uvTL;
				deltaUV2 = uvBR - uvTL;

				f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

				const glm::vec3 normal1 = glm::cross(edge1, edge2);
				const glm::vec3 tangent1 = f * (edge1 * deltaUV2.y - edge2 * deltaUV1.y);
				const glm::vec3 bitangent1 = f * (edge2 * deltaUV1.x - edge1 * deltaUV2.x);

				vertices[topLeft].Normal += normal1;
				vertices[bottomLeft].Normal += normal1;
				vertices[bottomRight].Normal += normal1;

				vertices[topLeft].Tangent += tangent1;
				vertices[bottomLeft].Tangent += tangent1;
				vertices[bottomRight].Tangent += tangent1;

				vertices[topLeft].Bitangent += bitangent1;
				vertices[bottomLeft].Bitangent += bitangent1;
				vertices[bottomRight].Bitangent += bitangent1;

				// Triangle 2
				edge1 = posBR - posTL;
				edge2 = posTR - posTL;
				deltaUV1 = uvBR - uvTL;
				deltaUV2 = uvTR - uvTL;

				f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

				const glm::vec3 normal2 = glm::cross(edge1, edge2);
				const glm::vec3 tangent2 = f * (edge1 * deltaUV2.y - edge2 * deltaUV1.y);
				const glm::vec3 bitangent2 = f * (edge2 * deltaUV1.x - edge1 * deltaUV2.x);

				vertices[topLeft].Normal += normal2;
				vertices[topRight].Normal += normal2;
				vertices[bottomRight].Normal += normal2;

				vertices[topLeft].Tangent += tangent2;
				vertices[topRight].Tangent += tangent2;
				vertices[bottomRight].Tangent += tangent2;

				vertices[topLeft].Bitangent += bitangent2;
				vertices[topRight].Bitangent += bitangent2;
				vertices[bottomRight].Bitangent += bitangent2;
			}
		}

		for (auto& vertex : vertices)
		{
			vertex.Normal = glm::normalize(vertex.Normal);
			vertex.Tangent = glm::normalize(vertex.Tangent);
			vertex.Bitangent = glm::normalize(vertex.Bitangent);
		}

		LandscapeMesh = Model::Create({ Mesh::Create(std::string(), vertices, indices, nullptr) });
	}

	void DistanceConstraintComponent::UpdateType()
	{
		if (Type == DistanceType::Fixed)
			Distance = 0.0f;
		else if (Type == DistanceType::Range)
		{
			MinDistance = 0.0f;
			MaxDistance = 0.0f;
		}
	}

	RackAndPinionConstraintComponent::RackAndPinionConstraintComponent()
	{
		UpdateMode();
	}

	void RackAndPinionConstraintComponent::UpdateMode()
	{
		if (Mode == RatioMode::Ratio)
			Ratio = 1.0f;
		else
		{
			RackTeethCount = 1;
			PinionTeethCount = 1;
			RackLength = 1.0f;
		}
	}

	SixDOFConstraintComponent::AxisStatus SixDOFConstraintComponent::GetAxisStatus(const Axis axis) const
	{
		if (LimitMin[axis] == -FLT_MAX && LimitMax[axis] == FLT_MAX)
			return AxisStatus::Free;

		if (LimitMin[axis] == FLT_MAX && LimitMax[axis] == -FLT_MAX)
			return AxisStatus::Locked;

		return AxisStatus::Custom;
	}

	void SixDOFConstraintComponent::SetAxisStatus(const Axis axis, const AxisStatus status)
	{
		switch (status)
		{
		case AxisStatus::Free:
		{
			LimitMin[axis] = -FLT_MAX;
			LimitMax[axis] =  FLT_MAX;
			break;
		}
		case AxisStatus::Locked:
		{
			LimitMin[axis] =  FLT_MAX;
			LimitMax[axis] = -FLT_MAX;
			break;
		}
		case AxisStatus::Custom:
		{
			LimitMin[axis] = 0.0f;
			LimitMax[axis] = 0.0f;
			break;
		}
		}
	}

}