#pragma once

#include "Dymatic/Asset/AssetSerializer.h"
#include "Dymatic/Renderer/Model.h"

namespace Dymatic {
	
	class MeshSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
		{
			YAML::Emitter out;
			out << YAML::BeginMap << YAML::Key << "Mesh" << YAML::Value << YAML::BeginMap;

			Ref<Model> model = As<Model>(asset);

			out << YAML::Key << "Source" << YAML::Value << model->GetSourceHandle();

			const Ref<Skeleton> skeleton = model->GetSkeleton();

			if (skeleton)
			{
				const AssetHandle skeletonHandle = skeleton->Handle;
				if (skeletonHandle)
					out << YAML::Key << "Skeleton" << YAML::Value << skeletonHandle;
			}

			const auto& lodInfo = model->GetLodInfo();
			if (!lodInfo.empty())
			{
				out << YAML::Key << "LODs" << YAML::BeginSeq;
				for (const auto& lod : lodInfo)
				{
					out << YAML::BeginMap;
					out << YAML::Key << "Screen Size" << YAML::Value << lod.ScreenSize;

					if (lod.UseAutoLOD)
						out << YAML::Key << "Reduction Factor" << YAML::Value << lod.LODReductionFactor;
					else
						out << YAML::Key << "Source" << YAML::Value << lod.SourceHandle;
					out << YAML::EndMap;
				}
				out << YAML::EndSeq; // LODs
			}
			
			out << YAML::EndMap; // Mesh
			out << YAML::EndMap;

			std::ofstream fout(Project::GetAssetFileSystemPath(metadata.FilePath));
			fout << out.c_str();

			return;
		}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset, bool multithreaded) const override
		{
			const std::filesystem::path filepath = AssetManager::GetFileSystemPathString(metadata);
			
			if (!std::filesystem::exists(filepath))
			{
				DY_CORE_ERROR("Mesh file '{}' does not exist", metadata.FilePath.string());
				return false;
			}

			YAML::Node data;
			try
			{
				data = YAML::LoadFile(filepath.string());
			}
			catch (YAML::ParserException e)
			{
				DY_CORE_ERROR("Failed to load mesh file '{}'\n     {}", metadata.FilePath.string(), e.what());
				return false;
			}

			auto meshNode = data["Mesh"];

			if (!meshNode)
				return false;

			AssetHandle sourceHandle = meshNode["Source"].as<AssetHandle>();
			Ref<Model> model;

			if (auto skeletonNode = meshNode["Skeleton"])
			{
				AssetHandle skeletonHandle = skeletonNode.as<AssetHandle>();
				Ref<Asset> skeletonAsset = AssetManager::GetAsset(skeletonHandle);
				model = Model::Create(sourceHandle, As<Skeleton>(skeletonAsset));
			}
			else
				model = Model::Create(sourceHandle, nullptr);

			bool result = model->IsLoaded();

			// Handle LOD setup
			if (auto lodsNode = meshNode["LODs"])
			{
				std::vector<Model::LODInfo> lodInfo;

				for (auto lodNode : lodsNode)
				{
					const auto screenSize = lodNode["Screen Size"];
					if (!screenSize)
						continue;

					auto& lod = lodInfo.emplace_back();
					lod.ScreenSize = screenSize.as<float>();

					if (auto sourceNode = lodNode["Source"])
					{
						lod.UseAutoLOD = false;
						lod.SourceHandle = sourceNode.as<AssetHandle>();
					}
					else
					{
						lod.UseAutoLOD = true;
						lod.LODReductionFactor = lodNode["Reduction Factor"].as<float>();
					}
				}

				model->SetLodInfo(lodInfo);
			}

			//if (!result)
			//	asset->SetFlag(AssetFlag::Invalid, true);

			asset = model;
			return result;
		}

		static void SerializeMeshToAssetPack(const Ref<Mesh> mesh, FileStreamWriter& stream)
		{
			// Write basic properties
			stream.WriteString(mesh->GetName());
			stream.WriteRaw<AABB>(mesh->GetAABB());

			// Write Vertices
			const auto& vertices = mesh->GetVerticies();
			stream.WriteRaw<size_t>(vertices.size());
			for (const auto& vertex : vertices)
				stream.WriteRaw<MeshVertex>(vertex);

			// Write Indices
			const auto& indices = mesh->GetIndicies();
			stream.WriteRaw<size_t>(indices.size());
			for (const auto index : indices)
				stream.WriteRaw<uint32_t>(index);
		}

		virtual bool SerializeToAssetPack(const AssetMetadata& metadata, const AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const override
		{
			Ref<Model> model = AssetManager::GetAsset<Model>(handle);

			// Write basic properties
			const Ref<Skeleton> skeleton = model->GetSkeleton();
			stream.WriteRaw<AssetHandle>(skeleton ? skeleton->Handle : 0);
			stream.WriteRaw<AABB>(model->GetAABB());

			// Write LOD Info
			const auto& lodInfo = model->GetLodInfo();
			stream.WriteRaw<uint32_t>(lodInfo.size());
			for (const auto& lod : lodInfo)
				stream.WriteRaw<float>(lod.ScreenSize);

			// Write Blend Shape Info
			const auto& blendShapes = model->GetBlendShapes();
			stream.WriteRaw<uint32_t>(blendShapes.size());
			for (const auto& blendShape : blendShapes)
				stream.WriteString(blendShape);

			// Write LOD 0
			const auto& meshes = model->GetMeshes();
			stream.WriteRaw<uint32_t>(meshes.size());
			for (const auto& mesh : meshes)
				SerializeMeshToAssetPack(mesh, stream);

			// Write Remaining LODs
			const auto& lods = model->GetLODs();
			stream.WriteRaw<uint32_t>(lods.size());
			for (const auto& meshes : lods)
			{
				stream.WriteRaw<uint32_t>(meshes.size());
				for (const auto& mesh : meshes)
					SerializeMeshToAssetPack(mesh, stream);
			}

			// Write Blend Shape GPU Data
			const Ref<ShaderStorageBuffer> blendShapeGPUBuffer = model->GetBlendShapeBuffer();
			const bool usingBlendShapes = (bool)blendShapeGPUBuffer;
			stream.WriteRaw<bool>(usingBlendShapes);

			if (usingBlendShapes)
			{
				Buffer data = blendShapeGPUBuffer->GetData();
				stream.WriteBuffer(data);
				data.Release();
			}

			return true;
		}

		static Ref<Mesh> DeserializeMeshFromAssetPack(FileStreamReader& stream)
		{
			// Basic Properties
			std::string name;
			stream.ReadString(name);

			AABB aabb;
			stream.ReadRaw<AABB>(aabb);

			// Vertices
			const size_t vertexCount = stream.ReadRaw<uint64_t>();
			std::vector<MeshVertex> vertices(vertexCount);
			for (size_t vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
				stream.ReadRaw<MeshVertex>(vertices[vertexIndex]);

			// Indices
			const size_t indexCount = stream.ReadRaw<uint64_t>();
			std::vector<uint32_t> indices(indexCount);
			for (size_t index = 0; index < indexCount; index++)
				stream.ReadRaw<uint32_t>(indices[index]);

			return Mesh::Create(name, aabb, vertices, indices, nullptr);
		}

		virtual bool DeserializeFromAssetPack(const AssetMetadata& metadata, Ref<Asset>& asset, FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const override
		{
			// Basic Properties
			const AssetHandle skeletonHandle = stream.ReadRaw<AssetHandle>();
			Ref<Skeleton> skeleton = AssetManager::GetAsset<Skeleton>(skeletonHandle);

			AABB aabb;
			stream.ReadRaw<AABB>(aabb);

			// LOD Info
			const uint32_t lodInfoCount = stream.ReadRaw<uint32_t>();
			std::vector<Model::LODInfo> lodInfo(lodInfoCount);
			for (uint32_t lodInfoIndex = 0; lodInfoIndex < lodInfoCount; lodInfoIndex++)
				stream.ReadRaw<float>(lodInfo[lodInfoIndex].ScreenSize);

			// Blend Shape Info
			const uint32_t blendShapeCount = stream.ReadRaw<uint32_t>();
			std::vector<std::string> blendShapes(blendShapeCount);
			for (uint32_t blendShapeIndex = 0; blendShapeIndex < blendShapeCount; blendShapeIndex++)
				stream.ReadString(blendShapes[blendShapeIndex]);

			// LOD 0
			const uint32_t meshCount = stream.ReadRaw<uint32_t>();
			std::vector<Ref<Mesh>> meshes(meshCount);
			for (uint32_t meshIndex = 0; meshIndex < meshCount; meshIndex++)
				 meshes[meshIndex] = DeserializeMeshFromAssetPack(stream);

			// Remaining LODs
			const uint32_t lodCount = stream.ReadRaw<uint32_t>();
			std::vector<std::vector<Ref<Mesh>>> lods(lodCount);
			for (uint32_t lodIndex = 0; lodIndex < lodCount; lodIndex++)
			{
				const uint32_t meshCount = stream.ReadRaw<uint32_t>();
				auto& meshes = lods[lodIndex] = std::vector<Ref<Mesh>>(meshCount);
				for (uint32_t meshIndex = 0; meshIndex < meshCount; meshIndex++)
					meshes[meshIndex] = DeserializeMeshFromAssetPack(stream);
			}

			// Blend Shape GPU Data
			Ref<ShaderStorageBuffer> blendShapeGPUBuffer = nullptr;
			if (stream.ReadRaw<bool>())
			{
				Buffer blendShapeGPUData;
				stream.ReadBuffer(blendShapeGPUData);
				blendShapeGPUBuffer = ShaderStorageBuffer::Create(blendShapeGPUData, ShaderStorageBufferUsage::STATIC_DRAW);
				blendShapeGPUData.Release();
			}

			Ref<Model> model = Model::Create(skeleton, aabb, lodInfo, blendShapes, meshes, lods, blendShapeGPUBuffer);
			asset = model;
			return true;
		}
	};

}