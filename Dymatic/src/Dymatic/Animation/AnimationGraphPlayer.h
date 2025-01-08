#pragma once

#include "Dymatic/Animation/AnimationGraph.h"
#include "Dymatic/Renderer/Model.h"

namespace Dymatic {

	typedef std::vector<glm::mat4> BoneMatrixList;
	typedef std::vector<float> BlendShapeWeightList;

	class AnimationGraphPlayer
	{
	public:
		static Ref<AnimationGraphPlayer> Create(Ref<Model> model, Ref<AnimationGraph> animationGraph) { return CreateRef<AnimationGraphPlayer>(model, animationGraph); }

		AnimationGraphPlayer(Ref<Model> model, Ref<AnimationGraph> animationGraph);

		void OnUpdate(Timestep ts);
		inline Ref<BoneMatrixList> GetFinalBoneMatrices() const { return m_FinalBoneMatrices; }
		inline const BoneMatrixList& GetGlobalBoneMatrices() const { return m_GlobalBoneMatrices; }
		inline Ref<BlendShapeWeightList> GetBlendShapeWeights() { return m_BlendShapeWeights; }

		inline Ref<AnimationGraph> GetAnimationGraph() const { return m_AnimationGraph; }

		inline const bool GetIsPaused() const { return m_Paused; }
		inline void SetIsPaused(const bool paused) { m_Paused = paused; }
		inline AnimationParameterMap& GetParameters() const { return m_Data->Parameters; }

		// Script Engine
		bool SetParameter(const std::string& name, const AnimationGraphData& value);
		bool DoesParameterExist(const std::string& name);
	private:
		void CheckCompilation();

	private:
		Ref<AnimationGraph> m_AnimationGraph;
		Ref<Model> m_Model;
		
		UUID m_CompileID;
		Ref<AnimationGraphInstanceData> m_Data;
		float m_CurrentTime = 0.0f;
		bool m_Paused = false;

		Ref<BoneMatrixList> m_FinalBoneMatrices;
		BoneMatrixList m_GlobalBoneMatrices;
		Ref<BlendShapeWeightList> m_BlendShapeWeights;
	};

}
