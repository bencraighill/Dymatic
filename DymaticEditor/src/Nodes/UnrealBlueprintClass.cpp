//Dymatic C++ Node Script - Source - V1.2.2
#include "UnrealBlueprintClass.h"
#include "CustomNodeLibrary.h"

namespace Dymatic {

	void UnrealBlueprintClass::OnCreate()
	{
		bpf__On_Create_7__pf();
	}

	void UnrealBlueprintClass::OnUpdate(Timestep ts)
	{
		bpf__On_Update_9__pf(ts.GetSeconds());
	}

	void UnrealBlueprintClass::bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_36(int32_t bpp__EntryPoint__pf)
	{
		
		check(bpp__EntryPoint__pf == 7);
		return; // Termination end of function
}
	void UnrealBlueprintClass::bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_37(int32_t bpp__EntryPoint__pf)
	{
	}
	void UnrealBlueprintClass::bpf__On_Create_7__pf()
	{
		bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_36(7);
	}

	void UnrealBlueprintClass::bpf__On_Update_9__pf(float Delta_Seconds)
	{
		bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_37(9);
	}

}
