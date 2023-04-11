#pragma once

#include "Dymatic/Asset/Asset.h"

#include "Dymatic/Renderer/Mesh.h"
#include "Dymatic/Renderer/AnimationData.h"
#include "Dymatic/Renderer/Shader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <map>

namespace Dymatic {

	class Model : public Asset
	{
	public:
		static AssetType GetStaticType() { return AssetType::Mesh; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	public:
		static Ref<Model> Create(const std::string& path) { return CreateRef<Model>(path); }

		Model(const std::string& path) { LoadModel(path); }

		auto& GetBoneInfoMap() { return m_BoneInfoMap; }
		uint32_t& GetBoneCount() { return m_BoneCounter; }

		inline bool IsLoaded() const { return m_IsLoaded; }
		inline const std::vector<Ref<Mesh>>& GetMeshes() const { return m_Meshes; }

		inline const std::unordered_map<std::string, float>& GetBlendShapeWeights() const { return m_BlendShapeWeights; }
		void UpdateBlendShapes();
		
	private:
		void LoadModel(const std::string& path);
		void ProcessNode(aiNode* node, const aiScene* scene);
		Ref<Mesh> ProcessMesh(aiMesh* mesh, const aiScene* scene);
		std::vector<Ref<Texture2D>> LoadMaterialTextures(aiMaterial* mat, aiTextureType type);

		void SetVertexBoneDataToDefault(MeshVertex& vertex);
		void SetVertexBoneData(MeshVertex& vertex, int boneID, float weight);
		void ExtractBoneWeightForVertices(std::vector<MeshVertex>& vertices, aiMesh* mesh, const aiScene* scene);

	private:
		std::map<std::string, BoneInfo> m_BoneInfoMap;
		uint32_t m_BoneCounter = 0;

		struct LoadData
		{
			Ref<Texture2D> texture;
			const char* path;
		};
		std::vector<LoadData> m_TexturesLoaded;

		std::vector<Ref<Mesh>> m_Meshes;
		std::string m_Path;
		
		std::unordered_map<std::string, float> m_BlendShapeWeights;

		bool m_IsLoaded = false;
	};

}