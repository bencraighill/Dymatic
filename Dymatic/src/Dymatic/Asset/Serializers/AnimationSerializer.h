#pragma once
#include "Dymatic/Asset/AssetSerializer.h"

#include "Dymatic/Animation/Animation.h"

namespace Dymatic {

	class AnimationSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
		{
			YAML::Emitter out;
			out << YAML::BeginMap << YAML::Key << "Animation" << YAML::Value << YAML::BeginMap;

			out << YAML::Key << "Source" << YAML::Value << As<Animation>(asset)->GetSourceHandle();

			out << YAML::Key << "Index" << YAML::Value << As<Animation>(asset)->GetAnimationIndex();

			const AssetHandle skeletonHandle = As<Animation>(asset)->GetSkeleton()->Handle;
			if (skeletonHandle)
				out << YAML::Key << "Skeleton" << YAML::Value << skeletonHandle;

			out << YAML::EndMap; // Animation
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
				DY_CORE_ERROR("Animation file '{}' does not exist", metadata.FilePath.string());
				return false;
			}

			YAML::Node data;
			try
			{
				data = YAML::LoadFile(filepath.string());
			}
			catch (YAML::ParserException e)
			{
				DY_CORE_ERROR("Failed to load animation file '{}'\n     {}", metadata.FilePath.string(), e.what());
				return false;
			}

			if (auto animationNode = data["Animation"])
			{
				AssetHandle sourceHandle = animationNode["Source"].as<AssetHandle>();

				uint32_t animationIndex = 0;
				if (auto animationIndexNode = animationNode["Index"])
					animationIndex = animationIndexNode.as<uint32_t>();
				
				if (auto skeletonNode = animationNode["Skeleton"])
				{
					AssetHandle skeletonHandle = skeletonNode.as<AssetHandle>();
					Ref<Asset> skeletonAsset = AssetManager::GetAsset(skeletonHandle);
					asset = Animation::Create(sourceHandle, animationIndex, As<Skeleton>(skeletonAsset));
				}
				else
				{
					DY_CORE_ERROR("Animation does not have a skeleton!");
					return false;
				}

				bool result = As<Animation>(asset)->IsLoaded();

				//if (!result)
				//	asset->SetFlag(AssetFlag::Invalid, true);

				return result;
			}

			return false;
		}

		virtual bool SerializeToAssetPack(const AssetMetadata& metadata, const AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const override
		{
			Ref<Animation> animation = AssetManager::GetAsset<Animation>(handle);

			// Serialize references and basic properties
			stream.WriteRaw<AssetHandle>(animation->GetSkeleton()->Handle);
			stream.WriteRaw<float>(animation->GetDuration());
			stream.WriteRaw<uint32_t>(animation->GetTicksPerSecond());

			// Serialize bones
			const auto& bones = animation->GetBones();
			stream.WriteRaw<uint32_t>(bones.size());
			for (const auto& [name, bone] : bones)
			{
				stream.WriteString(name);
				stream.WriteRaw<int>(bone->GetBoneID());

				// Write Position Keyframes
				const auto& positions = bone->GetPositions();
				stream.WriteRaw<uint32_t>(positions.size());
				for (const auto& position : positions)
				{
					stream.WriteRaw<glm::vec3>(position.position);
					stream.WriteRaw<float>(position.timeStamp);
				}

				// Write Rotation Keyframes
				const auto& rotations = bone->GetRotations();
				stream.WriteRaw<uint32_t>(rotations.size());
				for (const auto& rotation : rotations)
				{
					stream.WriteRaw<glm::quat>(rotation.orientation);
					stream.WriteRaw<float>(rotation.timeStamp);
				}

				// Write Scale Keyframes
				const auto& scales = bone->GetScales();
				stream.WriteRaw<uint32_t>(scales.size());
				for (const auto& scale : scales)
				{
					stream.WriteRaw<glm::vec3>(scale.scale);
					stream.WriteRaw<float>(scale.timeStamp);
				}
			}

			return true;
		}

		virtual bool DeserializeFromAssetPack(const AssetMetadata& metadata, Ref<Asset>& asset, FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const override
		{
			// References and basic properties
			Ref<Skeleton> skeleton = AssetManager::GetAsset<Skeleton>(stream.ReadRaw<AssetHandle>());

			if (!skeleton)
				return false;

			const float duration = stream.ReadRaw<float>();
			const uint32_t ticksPerSecond = stream.ReadRaw<uint32_t>();

			// Bones
			const uint32_t boneCount = stream.ReadRaw<uint32_t>();
			std::unordered_map<std::string, Ref<Bone>> bones(boneCount);
			for (uint32_t boneIndex = 0; boneIndex < boneCount; boneIndex++)
			{
				std::string name;
				stream.ReadString(name);
				const int id = stream.ReadRaw<int>();

				// Position Keyframes
				const uint32_t positionCount = stream.ReadRaw<uint32_t>();
				std::vector<KeyPosition> positions(positionCount);
				for (auto& position : positions)
				{
					stream.ReadRaw<glm::vec3>(position.position);
					stream.ReadRaw<float>(position.timeStamp);
				}

				// Rotation Keyframes
				const uint32_t rotationCount = stream.ReadRaw<uint32_t>();
				std::vector<KeyRotation> rotations(rotationCount);
				for (auto& rotation : rotations)
				{
					stream.ReadRaw<glm::quat>(rotation.orientation);
					stream.ReadRaw<float>(rotation.timeStamp);
				}

				// Scale Keyframes
				const uint32_t scaleCount = stream.ReadRaw<uint32_t>();
				std::vector<KeyScale> scales(scaleCount);
				for (auto& scale : scales)
				{
					stream.ReadRaw<glm::vec3>(scale.scale);
					stream.ReadRaw<float>(scale.timeStamp);
				}

				bones[name] = Bone::Create(name, id, positions, rotations, scales);
			}

			Ref<Animation> animation = Animation::Create(skeleton, duration, ticksPerSecond, bones);
			asset = animation;

			return true;
		}
	};

}