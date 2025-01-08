#include "dypch.h"
#include "Dymatic/Renderer/Model.h"

#include "Dymatic/Asset/AssetManager.h"

#include "Dymatic/Renderer/Utils/AssimpGLMHelpers.h"

#include "Dymatic/Renderer/LODGenerator.h"

#include "Dymatic/Math/StringUtils.h"

#include <assimp/version.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define INT_CEILING_DIVISION(dividend, divisor) ((dividend + divisor - 1) / divisor)

namespace Dymatic {

	Model::Model()
	{}

	Model::Model(const std::filesystem::path& path)
	{
		LoadBaseModel(path);
	}

	Model::Model(AssetHandle sourceHandle, Ref<Skeleton> skeleton, const bool load)
		: m_SourceHandle(sourceHandle), m_Skeleton(skeleton)
	{
		if (!load)
			return;

		LoadBaseModel(AssetManager::GetFileSystemPathString(AssetManager::GetMetadata(sourceHandle)));
	}

	Model::Model(const std::vector<Ref<Mesh>>& meshes)
		: m_Meshes(meshes)
	{
		SetupBaseModel();
	}

	Model::Model(Ref<Skeleton> skeleton, const AABB& aabb, const std::vector<LODInfo>& lodInfo, const std::vector<std::string>& blendShapes, const std::vector<Ref<Mesh>>& meshes, const std::vector<std::vector<Ref<Mesh>>>& lods, Ref<ShaderStorageBuffer> blendShapeBuffer)
		: m_Skeleton(skeleton), m_AABB(aabb), m_LODInfo(lodInfo), m_BlendShapes(blendShapes), m_Meshes(meshes), m_LODs(lods), m_BlendShapeBuffer(blendShapeBuffer)
	{
		CalculateVertexCount();
		m_IsLoaded = true;
	}

	void Model::LoadBaseModel(const std::filesystem::path& filepath)
	{
		LoadModel(filepath, m_Meshes, true);
		SetupBaseModel();
	}

	void Model::SetupBaseModel()
	{
		CalculateVertexCount();
		CalculateBoundingBox();
		BuildSDF();

		m_IsLoaded = true;
	}

	const std::vector<Ref<Mesh>>& Model::GetMeshes(const float screenSize) const
	{
		// Choose the LOD
		uint32_t lod = 0;

		for (auto& lodInfo : m_LODInfo)
		{
			if (lodInfo.ScreenSize < screenSize)
				break;

			lod++;
		}

		return GetMeshes(lod);
	}

	const std::vector<Ref<Mesh>>& Model::GetMeshes(uint32_t lod) const
	{
		if (lod == 0)
			return m_Meshes;

		return m_LODs[lod - 1];
	}

	void Model::SetLodInfo(const std::vector<LODInfo>& lodInfo)
	{
		m_LODInfo = lodInfo;
		GenerateLODs();
	}

	void Model::CreateLod(const LODInfo& lodInfo)
	{
		uint32_t lodIndex = 0;
		for (const auto& info : m_LODInfo)
		{
			if (info.ScreenSize < lodInfo.ScreenSize)
				break;

			lodIndex++;
		}

		// Insert the lodInfo into the desired position
		m_LODInfo.insert(m_LODInfo.begin() + lodIndex, lodInfo);
		m_LODs.insert(m_LODs.begin() + lodIndex, std::vector<Ref<Mesh>>());

		GenerateLOD(lodInfo, m_LODs[lodIndex]);
	}

	void Model::UpdateLod(uint32_t lod, const LODInfo& lodInfo)
	{
		DeleteLod(lod);
		CreateLod(lodInfo);
	}

	void Model::DeleteLod(uint32_t lod)
	{
		DY_CORE_ASSERT(lod != 0, "Cannot overwrite properties of LOD 0 (the original source mesh)");

		const uint32_t lodIndex = (lod - 1);
		m_LODInfo.erase(m_LODInfo.begin() + lodIndex);
		m_LODs.erase(m_LODs.begin() + lodIndex);
	}

	void Model::BuildSDF()
	{
		return;

		// Note: This is an editor-only shader routine and should not be executed at distribution runtime!
		TextureSpecification specification;
		specification.Width = RendererConstants::MeshSDFResolution;
		specification.Height = RendererConstants::MeshSDFResolution;
		specification.Depth = RendererConstants::MeshSDFResolution;
		specification.Format = TextureFormat::R32F;
		specification.SamplerWrap = TextureWrap::ClampToEdge;
		m_SDF = Texture3D::Create(specification);

		// Align C++ side AABB to GLSL padding requirements
		struct
		{
			glm::vec4 Min;
			glm::vec4 Max;
			uint32_t Resolution;
			uint32_t IndexCount;
		} sdfBufferData;

		sdfBufferData.Min = glm::vec4(m_AABB.Min, 1.0f);
		sdfBufferData.Max = glm::vec4(m_AABB.Max, 1.0f);
		sdfBufferData.Resolution = RendererConstants::MeshSDFResolution;

		m_SDF->BindTexture(0);
		const uint32_t invocations = INT_CEILING_DIVISION(RendererConstants::MeshSDFResolution, RendererConstants::SDFGenerationLocalSize);

		for (const auto& mesh : m_Meshes)
		{
			sdfBufferData.IndexCount = mesh->GetIndexCount();
			Renderer::SetEditorScratchBufferData(&sdfBufferData, sizeof(sdfBufferData));

			mesh->BindVertexBuffer(RendererConstants::Editor);
			mesh->BindIndexBuffer(RendererConstants::Editor + 1);
			Renderer::GetShaderLibrary()->Get("Renderer3D_GenerateSDF")->Dispatch(invocations, invocations, invocations);
		}
	}

	Ref<Model> Model::Copy(const bool copyLODs) const
	{
		std::vector<Ref<Mesh>> meshes;
		meshes.reserve(m_Meshes.size());
		for (const auto& mesh : m_Meshes)
			meshes.push_back(mesh->Copy());

		std::vector<std::vector<Ref<Mesh>>> lods;
		if (copyLODs)
		{
			lods.reserve(m_LODs.size());
			for (const auto& lod : m_LODs)
			{
				std::vector<Ref<Mesh>>& newLods = lods.emplace_back();
				for (const auto& mesh : lod)
					newLods.push_back(mesh->Copy());
			}
		}

		return Model::Create(m_Skeleton, m_AABB, copyLODs ? m_LODInfo : std::vector<LODInfo>(), m_BlendShapes, meshes, lods, m_BlendShapeBuffer);
	}

	void Model::LoadModel(const std::filesystem::path& filepath, std::vector<Ref<Mesh>>& meshes, bool processBlendshapes)
	{
		// TODO: Look at these assimp flags carefully, can optimize meshes which share materials to be merged together!
		Assimp::Importer importer;
		importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 100.0f);
		const aiScene* scene = importer.ReadFile(filepath.string(), aiProcess_GlobalScale | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			DY_CORE_ERROR("ASSIMP: {}", importer.GetErrorString());
			return;
		}

		uint32_t globalIndex = 0;
		std::unordered_map<std::string, std::vector<glm::vec3>> morphMap;
		ProcessNode(meshes, scene->mRootNode, scene, globalIndex, morphMap, processBlendshapes);

		// Once the morph map has been populated, generate the GPU blend shape buffer
		// Note: This should only occur for the base (level 0) LOD!
		if (!morphMap.empty())
		{
			std::vector<glm::vec3> blendshapeData;

			m_BlendShapes.reserve(morphMap.size());
			for (const auto& [name, data] : morphMap)
			{
				m_BlendShapes.push_back(name);
				blendshapeData.insert(blendshapeData.end(), data.begin(), data.end());
			}

			const size_t bufferSize = blendshapeData.size() * sizeof(glm::vec3);
			m_BlendShapeBuffer = ShaderStorageBuffer::Create(bufferSize, ShaderStorageBufferUsage::STATIC_DRAW);
			m_BlendShapeBuffer->SetData(blendshapeData.data(), bufferSize);
			DY_CORE_WARN("Generated GPU blend shape buffer of size {}", String::FormatBytes(bufferSize));
		}
	}

	void Model::ProcessNode(std::vector<Ref<Mesh>>& meshes, aiNode* node, const aiScene* scene, uint32_t& globalIndex, std::unordered_map<std::string, std::vector<glm::vec3>>& morphMap, bool processBlendshapes)
	{
		meshes.reserve(node->mNumMeshes);

		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(ProcessMesh(mesh, scene, globalIndex, morphMap, processBlendshapes));
		}

		for (uint32_t i = 0; i < node->mNumChildren; i++)
			ProcessNode(meshes, node->mChildren[i], scene, globalIndex, morphMap, processBlendshapes);
	}

	Ref<Mesh> Model::ProcessMesh(aiMesh* mesh, const aiScene* scene, uint32_t& globalIndex, std::unordered_map<std::string, std::vector<glm::vec3>>& morphMap, bool processBlendshapes)
	{	
		// Process vertices
		std::vector<MeshVertex> vertices;
		vertices.reserve(mesh->mNumVertices);

		for (uint32_t i = 0; i < mesh->mNumVertices; i++)
		{
			MeshVertex vertex;
			SetVertexBoneDataToDefault(vertex);

			// Global Model Index
			vertex.GlobalIndex = globalIndex++;

			// Positions
			vertex.Position = AssimpGLMHelpers::GetGLMVec(mesh->mVertices[i]);
			
			// Normals
			if (mesh->HasNormals())
				vertex.Normal = AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]);

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

			vertex.Color = glm::vec4(0.0f);

			vertices.push_back(vertex);
		}

		// Process indices
		std::vector<uint32_t> indices;
		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (uint32_t j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

		// Process blend shapes
		for (uint32_t i = 0; i < mesh->mNumAnimMeshes; i++)
		{
			aiAnimMesh* animMesh = mesh->mAnimMeshes[i];
			const std::string blendShapeName = animMesh->mName.C_Str();

			// Compute the blend shape delta and append the vertex
			// Note: We assume here that assimp will also include vertices without blend shape data (which we append) so that the result morph map contains
			// as many positions as there are vertices.
			for (uint32_t i = 0; i < animMesh->mNumVertices; i++)
				morphMap[blendShapeName].push_back(AssimpGLMHelpers::GetGLMVec(animMesh->mVertices[i]) - vertices[i].Position);
		}

		//// Process Materials
		//aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		//Ref<Material> mat = Material::Create((*material->mProperties)->mKey.C_Str());
		//aiColor4D color = {};
		//
		//material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
		//mat->SetAlbedo(glm::vec4(color.r, color.g, color.b, 1.0f));
		//
		//{
		//	auto diffuseMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE);
		//	if (!diffuseMaps.empty())
		//		mat->SetAlbedoMap(diffuseMaps[0]);
		//}
		//{
		//	auto alphaMaps = LoadMaterialTextures(material, aiTextureType_OPACITY);
		//	if (!alphaMaps.empty())
		//	{
		//		mat->SetAlbedoMap(alphaMaps[0]);
		//		mat->SetAlphaBlendMode(Material::AlphaBlendMode::Dithered);
		//	}
		//}

		ExtractBoneWeightForVertices(vertices, mesh, scene);

		return Mesh::Create(mesh->mName.C_Str(), vertices, indices, nullptr);
	}

	void Model::SetVertexBoneData(MeshVertex& vertex, int boneID, float weight)
	{
		for (uint32_t i = 0; i < MAX_BONE_INFLUENCE; i++)
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
		const auto& boneInfoMap = m_Skeleton->GetBoneInfoMap();

		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++)
		{
			const std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

			if (boneInfoMap.find(boneName) == boneInfoMap.end())
			{
				DY_CORE_WARN("Mesh contains weights for bone '{}' which does not exist on target skeleton!", boneName);
				continue;
			}

			const int boneID = boneInfoMap.at(boneName).id;

			DY_CORE_ASSERT(boneID != -1);
			
			const aiVertexWeight* weights = mesh->mBones[boneIndex]->mWeights;
			int numWeights = mesh->mBones[boneIndex]->mNumWeights;

			for (int weightIndex = 0; weightIndex < numWeights; weightIndex++)
			{
				const int vertexId = weights[weightIndex].mVertexId;
				const float weight = weights[weightIndex].mWeight;
				
				DY_CORE_ASSERT(vertexId <= vertices.size());
				
				SetVertexBoneData(vertices[vertexId], boneID, weight);
			}
		}
	}

	void Model::CalculateBoundingBox()
	{
		m_AABB = AABB();

		for (const auto& mesh : m_Meshes)
			m_AABB.Extend(mesh->GetAABB());
	}

	void Model::CalculateVertexCount()
	{
		// Cache this model's LOD 0 vertex count
		m_VertexCount = 0;
		for (const auto& mesh : m_Meshes)
			m_VertexCount += mesh->GetVerticies().size();
	}

	void Model::GenerateLODs()
	{
		m_LODs.clear();
		m_LODs.resize(m_LODInfo.size());

		for (size_t lodIndex = 0; lodIndex < m_LODInfo.size(); lodIndex++)
			GenerateLOD(m_LODInfo[lodIndex], m_LODs[lodIndex]);
	}

	void Model::GenerateLOD(const LODInfo& lodInfo, std::vector<Ref<Mesh>>& lodMeshes)
	{
		if (lodInfo.UseAutoLOD)
		{
			const double threashold = 0.1;
			const size_t meshCount = m_Meshes.size();
			lodMeshes.resize(meshCount);

			for (size_t i = 0; i < meshCount; i++)
			{
				auto sourceVertices = m_Meshes[i]->GetVerticies();
				auto sourceIndices = m_Meshes[i]->GetIndicies();

				// Generate LODs by simplifying the source mesh
				LODGenerator lodGenerator(sourceVertices, sourceIndices);
				size_t remain = (size_t)((sourceIndices.size() / 3) * lodInfo.LODReductionFactor);
				lodGenerator.Simplify(remain, threashold);
				
				// Unpack to generation result
				std::vector<MeshVertex> vertices;
				std::vector<uint32_t> indices;
				lodGenerator.UnpackResult(vertices, indices);

				// Create a mesh and assign to the LOD
				lodMeshes[i] = Mesh::Create(fmt::format("{}_LOD{}", m_Meshes[i]->GetName(), i), vertices, indices, nullptr);
			}
		}
		else
		{
			LoadModel(AssetManager::GetFileSystemPathString(lodInfo.SourceHandle), lodMeshes, false);
		}
	}

	void Model::SetVertexBoneDataToDefault(MeshVertex& vertex)
	{
		for (uint32_t i = 0; i < MAX_BONE_INFLUENCE; i++)
		{
			vertex.m_BoneIDs[i] = -1;
			vertex.m_Weights[i] = 0.0f;
		}
	}

}