#pragma once
//Dymatic C++ Node Script - Header - V1.2.2

#include <Dymatic/Scene/ScriptableEntity.h>
#include "NodeCore.h"

namespace Dymatic {

	class UnrealBlueprintClass : public ScriptableEntity
	{
	public:
		bool bpv__Test_Bool_89__pf;
		float bpv__Test_Float_90__pf;
		int bpv__Test_Int_91__pf;
		UnrealBlueprintClass();
		DYFUNCTION(BlueprintCallable)
		virtual void OnCreate() override;
		DYFUNCTION(BlueprintCallable)
		virtual void OnDestroy() override;
		DYFUNCTION(BlueprintCallable)
		virtual void OnUpdate(Timestep ts) override;
		void bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_98(int32_t bpp__EntryPoint__pf);
		void bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_99(int32_t bpp__EntryPoint__pf);
		DYFUNCTION(BlueprintCallable)
		virtual void bpf__On_Create_84__pf();
		float b0l__NodeEvent_bpf__On_Update_86__pf_Delta_Seconds__pf{};
		DYFUNCTION(BlueprintCallable)
		virtual void bpf__On_Update_86__pf(float Delta_Seconds);
		void bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_100(int32_t bpp__EntryPoint__pf);
		float b0l__NodeEvent_bpf__On_Update_94__pf_Delta_Seconds__pf{};
		DYFUNCTION(BlueprintCallable)
		virtual void bpf__On_Update_94__pf(float Delta_Seconds);
	};

}
