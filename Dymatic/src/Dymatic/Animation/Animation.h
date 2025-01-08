#pragma once

#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <assimp/scene.h>

#include <functional>

#include "Dymatic/Renderer/Bone.h"
#include "Dymatic/Animation/AnimationData.h"
#include "Dymatic/Renderer/Skeleton.h"

struct aiNode;
struct aiAnimation;

namespace Dymatic {

	class Animation : public Asset
	{
	public:
		static AssetType GetStaticType() { return AssetType::Animation; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
		
	public:
		static Ref<Animation> Create(const AssetHandle sourceHandle, uint32_t animationIndex, Ref<Skeleton> skeleton) { return CreateRef<Animation>(sourceHandle, animationIndex, skeleton); }
		static Ref<Animation> Create(Ref<Skeleton> skeleton, const float duration, const uint32_t ticksPerSecond, const std::unordered_map<std::string, Ref<Bone>>& bones) { return CreateRef<Animation>(skeleton, duration, ticksPerSecond, bones); }
		
	public:
		Animation(const AssetHandle sourceHandle, uint32_t animationIndex, Ref<Skeleton> skeleton);
		Animation(Ref<Skeleton> skeleton, const float duration, const uint32_t ticksPerSecond, const std::unordered_map<std::string, Ref<Bone>>& bones);
		~Animation() {}

		Ref<Bone> FindBone(const std::string& name);
		
		inline AssetHandle GetSourceHandle() const { return m_SourceHandle; }
		inline uint32_t GetAnimationIndex() const { return m_AnimationIndex; }
		inline const Ref<Skeleton> GetSkeleton() const { return m_Skeleton; }

		inline float GetTicksPerSecond() const { return m_TicksPerSecond; }
		inline float GetDuration() const { return m_Duration; }
		inline const std::unordered_map<std::string, Ref<Bone>>& GetBones() const { return m_Bones; }

		inline bool IsLoaded() { return m_IsLoaded; }

	private:
		void ReadMissingBones(const aiAnimation* animation);
		void ReadHeirarchyData(BoneNodeData& dest, const aiNode* src);

	private:
		Ref<Skeleton> m_Skeleton;
		uint32_t m_AnimationIndex = 0;
		AssetHandle m_SourceHandle = 0;

		float m_Duration;
		uint32_t m_TicksPerSecond;
		std::unordered_map<std::string, Ref<Bone>> m_Bones;

		bool m_IsLoaded = false;
	};

}