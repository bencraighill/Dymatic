#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <map>

#include "Dymatic/Renderer/Mesh.h"
#include "Dymatic/Renderer/AnimationData.h"
#include "Dymatic/Renderer/Shader.h"

namespace Dymatic {

	class Model
	{
	public:

		static Ref<Model> Create(const std::string& path) { return CreateRef<Model>(path); }

		Model(const std::string& path)
		{
			LoadModel(path);
		}

		void Draw(Ref<Shader> shader, bool bind_material = true);

		auto& GetBoneInfoMap() { return m_BoneInfoMap; }
		int& GetBoneCount() { return m_BoneCounter; }

		inline bool IsLoaded() const { return m_IsLoaded; }
		inline std::vector<Ref<Mesh>> GetMeshes() const { return m_Meshes; }

		inline const std::string GetPath() const { return m_Path; }

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
		int m_BoneCounter = 0;

		struct LoadData
		{
			Ref<Texture2D> texture;
			const char* path;
		};
		std::vector<LoadData> m_TexturesLoaded;

		std::vector<Ref<Mesh>> m_Meshes;
		std::string m_Path;

		bool m_IsLoaded = false;
	};

}