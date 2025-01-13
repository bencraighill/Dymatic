#pragma once

#include "Dymatic/Asset/Asset.h"

#include "Dymatic/Renderer/Mesh.h"
#include "Dymatic/Renderer/Shader.h"
#include "Dymatic/Renderer/ShaderStorageBuffer.h"

#include "Dymatic/Renderer/Skeleton.h"

#include <map>

struct aiScene;
struct aiNode;
struct aiMesh;
struct aiMaterial;
enum aiTextureType;

namespace Dymatic {

	class Model : public Asset
	{
	public:
		struct LODInfo
		{
			bool UseAutoLOD = true;

			float ScreenSize = 1.0;

			union
			{
				float LODReductionFactor = 1.0;
				AssetHandle SourceHandle;
			};
		};

	public:
		static AssetType GetStaticType() { return AssetType::Mesh; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	public:
		static Ref<Model> Create() { return CreateRef<Model>(); }
		static Ref<Model> Create(const std::filesystem::path& path) { return CreateRef<Model>(path); }
		static Ref<Model> Create(AssetHandle sourceHandle, Ref<Skeleton> skeleton, const bool load = true) { return CreateRef<Model>(sourceHandle, skeleton, load); }
		static Ref<Model> Create(const std::vector<Ref<Mesh>>& meshes) { return CreateRef<Model>(meshes); }
		static Ref<Model> Create(Ref<Skeleton> skeleton, const AABB& aabb, const std::vector<LODInfo>& lodInfo, const std::vector<std::string>& blendShapes, const std::vector<Ref<Mesh>>& meshes, const std::vector<std::vector<Ref<Mesh>>>& lods, Ref<ShaderStorageBuffer> blendShapeBuffer) { return CreateRef<Model>(skeleton, aabb, lodInfo, blendShapes, meshes, lods, blendShapeBuffer); }

		Model();
		Model(const std::filesystem::path& path);
		Model(AssetHandle sourceHandle, Ref<Skeleton> skeleton, const bool load);
		Model(const std::vector<Ref<Mesh>>& meshes);
		Model(Ref<Skeleton> skeleton, const AABB& aabb, const std::vector<LODInfo>& lodInfo, const std::vector<std::string>& blendShapes, const std::vector<Ref<Mesh>>& meshes, const std::vector<std::vector<Ref<Mesh>>>& lods, Ref<ShaderStorageBuffer> blendShapeBuffer);
		
		inline AssetHandle GetSourceHandle() const { return m_SourceHandle; }
		inline Ref<Skeleton> GetSkeleton() const { return m_Skeleton; }

		inline bool IsLoaded() const { return m_IsLoaded; }

		const std::vector<Ref<Mesh>>& GetMeshes(uint32_t lod = 0) const;
		const std::vector<Ref<Mesh>>& GetMeshes(const float screenSize) const;

		inline const std::vector<LODInfo>& GetLodInfo() const { return m_LODInfo; }
		void SetLodInfo(const std::vector<LODInfo>& lodInfo);
		void CreateLod(const LODInfo& lodInfo);
		void UpdateLod(uint32_t lod, const LODInfo& lodInfo);
		void DeleteLod(uint32_t lod);

		inline Ref<Texture3D> GetSDF() const { return m_SDF; }
		void BuildSDF();

		// Note: This is the vertex count for LOD 0.
		inline uint32_t GetVertexCount() const { return m_VertexCount; }
		inline const std::vector<std::string>& GetBlendShapes() const { return m_BlendShapes; }
		inline Ref<ShaderStorageBuffer> GetBlendShapeBuffer() const { return m_BlendShapeBuffer; }

		inline const AABB& GetAABB() const { return m_AABB; }

		// Modification Only (for dynamic physics runtime systems)
		Ref<Model> Copy(const bool copyLODs) const;
		inline std::vector<Ref<Mesh>>& GetMeshesEditable() { return m_Meshes; }

		// Serialization Only
		inline const std::vector<std::vector<Ref<Mesh>>>& GetLODs() const { return m_LODs; }
		
	private:
		void LoadBaseModel(const std::filesystem::path& filepath);
		void SetupBaseModel();

		void LoadModel(const std::filesystem::path& filepath, std::vector<Ref<Mesh>>& meshes, bool processBlendshapes);
		void ProcessNode(std::vector<Ref<Mesh>>& meshes, aiNode* node, const aiScene* scene, uint32_t& globalIndex, std::unordered_map<std::string, std::vector<glm::vec3>>& morphMap, bool processBlendshapes);
		Ref<Mesh> ProcessMesh(aiMesh* mesh, const aiScene* scene, uint32_t& globalIndex, std::unordered_map<std::string, std::vector<glm::vec3>>& morphMap, bool processBlendshapes);

		void CalculateBoundingBox();
		void CalculateVertexCount();
		void GenerateLODs();
		void GenerateLOD(const LODInfo& lodInfo, std::vector<Ref<Mesh>>& meshes);

		void SetVertexBoneDataToDefault(MeshVertex& vertex);
		void SetVertexBoneData(MeshVertex& vertex, int boneID, float weight);
		void ExtractBoneWeightForVertices(std::vector<MeshVertex>& vertices, aiMesh* mesh, const aiScene* scene);

	private:
		Ref<Skeleton> m_Skeleton;
		AssetHandle m_SourceHandle = 0;

		// Base (LOD 0) mesh
		std::vector<Ref<Mesh>> m_Meshes;

		// Meshes (structure as <LOD - 1, meshes>)
		std::vector<std::vector<Ref<Mesh>>> m_LODs;

		AABB m_AABB;
		std::vector<LODInfo> m_LODInfo;
		glm::vec3 m_ImportScale = glm::vec3(1.0f);

		uint32_t m_VertexCount;
		std::vector<std::string> m_BlendShapes;
		Ref<ShaderStorageBuffer> m_BlendShapeBuffer = nullptr;

		Ref<Texture3D> m_SDF = nullptr;

		bool m_IsLoaded = false;
	};

}