#pragma once
//Dymatic C++ Node Script - Header - V1.2.2

#include <Dymatic/Scene/ScriptableEntity.h>
#include "NodeCore.h"

namespace Dymatic {

	class UnrealBlueprintClass : public ScriptableEntity
	{
	public:
		DYFUNCTION(BlueprintCallable)
		virtual void OnCreate() override;
		DYFUNCTION(BlueprintCallable)
		virtual void OnUpdate(Timestep ts) override;
		void bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_36(int32_t bpp__EntryPoint__pf);
		void bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_37(int32_t bpp__EntryPoint__pf);
		DYFUNCTION(BlueprintCallable)
		virtual void bpf__On_Create_7__pf();
		float b0l__NodeEvent_bpf__On_Update_9__pf_Delta_Seconds__pf;
		DYFUNCTION(BlueprintCallable)
		virtual void bpf__On_Update_9__pf(float Delta_Seconds);
	};

}
