#include "dypch.h"
#include "Dymatic/Renderer/Model.h"

#include "Dymatic/Renderer/AssimpGLMHelpers.h"

namespace Dymatic {

	void Model::Draw(Ref<Shader> shader, bool bind_material)
	{
		shader->Bind();
		for (auto& mesh : m_Meshes)
			mesh->Draw(bind_material);
	}

	void Model::LoadModel(const std::string& path)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			DY_CORE_ERROR("ASSIMP: {0}", importer.GetErrorString());
			return;
		}

		m_Directory = path.substr(0, path.find_last_of("/\\"));

		ProcessNode(scene->mRootNode, scene);

		m_IsLoaded = true;
	}

	void Model::ProcessNode(aiNode* node, const aiScene* scene)
	{
		for (size_t i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			m_Meshes.push_back(ProcessMesh(mesh, scene));
		}

		for (size_t i = 0; i < node->mNumChildren; i++)
			ProcessNode(node->mChildren[i], scene);
	}

	Ref<Mesh> Model::ProcessMesh(aiMesh* mesh, const aiScene* scene)
	{
		std::vector<MeshVertex> verticies;
		std::vector<uint32_t> indicies;

		for (size_t i = 0; i < mesh->mNumVertices; i++)
		{
			MeshVertex vertex;
			SetVertexBoneDataToDefault(vertex);

			// Positions
			vertex.Position = AssimpGLMHelpers::GetGLMVec(mesh->mVertices[i]);
			
			// Normals
			if (mesh->HasNormals())
			{
				vertex.Normal = AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]);
			}

			// Texture Coords
			if (mesh->mTextureCoords[0])
			{
				glm::vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;

				// Tangent
				vertex.Tangent = AssimpGLMHelpers::GetGLMVec(mesh->mTangents[i]);

				// Bitangent
				vertex.Bitangent = AssimpGLMHelpers::GetGLMVec(mesh->mBitangents[i]);
			}
			else
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);

			verticies.push_back(vertex);
		}

		for (size_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (size_t j = 0; j < face.mNumIndices; j++)
				indicies.push_back(face.mIndices[j]);
		}

		// Process Materials
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		Ref<Material> mat = Material::Create();
		aiColor4D color = {};

		material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
		mat->SetAlbedo(glm::vec4(color.r, color.g, color.b, 1.0f));

		//auto baseColorMaps = LoadMaterialTextures(material, aiTextureType_BASE_COLOR);
		//if (!baseColorMaps.empty())
		//	mat->SetAlbedoMap(baseColorMaps[0]);
		//else
		{
			auto diffuseMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE);
			if (!diffuseMaps.empty())
				mat->SetAlbedoMap(diffuseMaps[0]);
		}

		ExtractBoneWeightForVertices(verticies, mesh, scene);

		return Mesh::Create(mesh->mName.C_Str(), verticies, indicies, mat);
	}

	void Model::SetVertexBoneData(MeshVertex& vertex, int boneID, float weight)
	{
		for (size_t i = 0; i < MAX_BONE_INFLUENCE; ++i)
		{
			if (vertex.m_BoneIDs[i] < 0)
			{
				vertex.m_Weights[i] = weight;
				vertex.m_BoneIDs[i] = boneID;
				break;
			}
		}
	}

	void Model::ExtractBoneWeightForVertices(std::vector<MeshVertex>& vertices, aiMesh* mesh, const aiScene* scene)
	{
		for (size_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
		{
			int boneID = -1;
			std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
			if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
			{
				BoneInfo newBoneInfo;
				newBoneInfo.id = m_BoneCounter;
				newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
				m_BoneInfoMap[boneName] = newBoneInfo;
				boneID = m_BoneCounter;
				m_BoneCounter++;
			}
			else
			{
				boneID = m_BoneInfoMap[boneName].id;
			}
			DY_CORE_ASSERT(boneID != -1);
			auto weights = mesh->mBones[boneIndex]->mWeights;
			int numWeights = mesh->mBones[boneIndex]->mNumWeights;

			for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
			{
				int vertexId = weights[weightIndex].mVertexId;
				float weight = weights[weightIndex].mWeight;
				DY_CORE_ASSERT(vertexId <= vertices.size());
				SetVertexBoneData(vertices[vertexId], boneID, weight);
			}
		}
	}

	std::vector<Ref<Texture2D>> Model::LoadMaterialTextures(aiMaterial* mat, aiTextureType type)
	{
		std::vector<Ref<Texture2D>> textures;
		for (size_t i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);

			// Check if texture has been loaded already.
			bool skip = false;
			for (size_t j = 0; j < m_TexturesLoaded.size(); j++)
			{
				if (std::strcmp(m_TexturesLoaded[j].path, str.C_Str()) == 0)
				{
					textures.push_back(m_TexturesLoaded[j].texture);
					skip = true;
					break;
				}
			}

			// Load texture if it hasn't.
			if (!skip)
			{
				Ref<Texture2D> texture = Texture2D::Create(m_Directory + '/' + (str.C_Str() + 4));
				if (texture->IsLoaded())
				{
					textures.push_back(texture);
					m_TexturesLoaded.push_back({ texture, str.C_Str() });
				}
			}
		}
		return textures;
	}

	void Model::SetVertexBoneDataToDefault(MeshVertex& vertex)
	{
		for (size_t i = 0; i < MAX_BONE_INFLUENCE; i++)
		{
			vertex.m_BoneIDs[i] = -1;
			vertex.m_Weights[i] = 0.0f;
		}
	}

}