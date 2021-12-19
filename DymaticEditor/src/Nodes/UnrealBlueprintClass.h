#pragma once
//Dymatic C++ Node Script - Header - V1.2.2

#include <Dymatic/Scene/ScriptableEntity.h>
#include "NodeCore.h"

namespace Dymatic {

	class UnrealBlueprintClass : public ScriptableEntity
	{
	public:
		bool bpv__Test_Bool_30__pf;
		float bpv__Test_Float_31__pf;
		int bpv__Test_Int_32__pf;
		UnrealBlueprintClass();
		DYFUNCTION(BlueprintCallable)
		virtual void OnCreate() override;
		DYFUNCTION(BlueprintCallable)
		virtual void OnDestroy() override;
		DYFUNCTION(BlueprintCallable)
		virtual void OnUpdate(Timestep ts) override;
		void bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_54(int32_t bpp__EntryPoint__pf);
		void bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_55(int32_t bpp__EntryPoint__pf);
		void bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_56(int32_t bpp__EntryPoint__pf);
		DYFUNCTION(BlueprintCallable)
		virtual void bpf__On_Create_25__pf();
		float b0l__NodeEvent_bpf__On_Update_27__pf_Delta_Seconds__pf{};
		DYFUNCTION(BlueprintCallable)
		virtual void bpf__On_Update_27__pf(float Delta_Seconds);
		DYFUNCTION(BlueprintCallable)
		virtual void bpf__On_Destroy_33__pf();
	};

}
