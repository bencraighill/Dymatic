//Dymatic C++ Node Script - Source - V1.2.2
#include "UnrealBlueprintClass.h"
#include "CustomNodeLibrary.h"

namespace Dymatic {

	UnrealBlueprintClass::UnrealBlueprintClass()
	{
		bpv__Test_Bool_89__pf = true;
		bpv__Test_Float_90__pf = 5.423000f;
		bpv__Test_Int_91__pf = 12;
	}

	void UnrealBlueprintClass::OnCreate()
	{
		bpf__On_Create_84__pf();
	}

	void UnrealBlueprintClass::OnDestroy()
	{
	}

	void UnrealBlueprintClass::OnUpdate(Timestep ts)
	{
		bpf__On_Update_86__pf(ts.GetSeconds());
		bpf__On_Update_94__pf(ts.GetSeconds());
	}

	void UnrealBlueprintClass::bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_98(int32_t bpp__EntryPoint__pf)
	{
		
		check(bpp__EntryPoint__pf == 84);
		return; // Termination end of function
}
	void UnrealBlueprintClass::bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_99(int32_t bpp__EntryPoint__pf)
	{
		
		check(bpp__EntryPoint__pf == 86);
		return; // Termination end of function
}
	void UnrealBlueprintClass::bpf__On_Create_84__pf()
	{
		bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_98(84);
	}

	void UnrealBlueprintClass::bpf__On_Update_86__pf(float Delta_Seconds)
	{
		bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_99(86);
	}

	void UnrealBlueprintClass::bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_100(int32_t bpp__EntryPoint__pf)
	{
		
		check(bpp__EntryPoint__pf == 94);
		return; // Termination end of function
}
	void UnrealBlueprintClass::bpf__On_Update_94__pf(float Delta_Seconds)
	{
		bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_100(94);
	}

}
