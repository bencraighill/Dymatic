#pragma once

#include "Dymatic/Asset/Asset.h"
#include "Dymatic/Animation/AnimationNode.h"
#include "Dymatic/Animation/AnimationGraphData.h"

#include "Dymatic/Core/Timestep.h"
#include "Dymatic/Renderer/Skeleton.h"

// Editor Only
#include "Dymatic/Animation/Editor/AnimationEditorGraph.h"

namespace Dymatic {

	// Editor Only
	namespace Editor {
		class AnimationGraphCompiler;
	}

	class AnimationGraph : public Asset
	{
	public:
		static Ref<AnimationGraph> Create(Ref<Skeleton> skeleton, Ref<Editor::AnimationEditorGraph> editorGraph) { return CreateRef<AnimationGraph>(skeleton, editorGraph); }

		static AssetType GetStaticType() { return AssetType::AnimationGraph; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	public:
		AnimationGraph(Ref<Skeleton> skeleton, Ref<Editor::AnimationEditorGraph> editorGraph);
		AnimationGraph& AnimationGraph::operator=(const AnimationGraph& other);

		const std::vector<glm::mat4>& GetFinalBoneMatrices(float time, const Ref<AnimationGraphInstanceData> activeData);
		void ResetNodes(const Ref<AnimationGraphInstanceData> activeData);

		inline Ref<AnimationPoseNode> GetOutputNode() const { return m_Output; }
		inline void SetOutputNode(Ref<AnimationPoseNode> output) { m_Output = output; }

		inline Ref<Skeleton> GetSkeleton() const { return m_Skeleton; }

		inline const UUID GetCompileID() const { return m_CompileID; }
		inline const AnimationParameterMap& GetDefaultParameters() const { return m_DefaultParameters; }
		inline const Ref<AnimationGraphInstanceData> GetActiveData() const { return m_ActiveData; }

		inline AnimationGraphInstanceData::InstanceValue& GetInstanceValue(const AnimationGraphValueHandle value) { return m_ActiveData->GetValue(value); }
		inline float GetInstanceDeltaTime() const { return m_ActiveData->DeltaTime; }

		inline bool IsValid() const { return m_Skeleton && m_Output; }

	private:

	private:
		Ref<Skeleton> m_Skeleton = nullptr;
		Ref<AnimationPoseNode> m_Output = nullptr;
		AnimationParameterMap m_DefaultParameters;
		Ref<AnimationGraphInstanceData> m_ActiveData = nullptr;

		std::vector<AnimationNode*> m_RegisteredNodes;

		// Editor Only
		UUID m_CompileID;
		Ref<Editor::AnimationEditorGraph> m_EditorGraph;

		friend class Editor::AnimationGraphCompiler;
		friend class AnimationGraphSerializer;
		friend class AnimationGraphPanel;
		friend class AnimationNode;
	};

}