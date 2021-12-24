//Dymatic C++ Node Script - Source - V1.2.2
#include "UnrealBlueprintClass.h"
#include "CustomNodeLibrary.h"

namespace Dymatic {

	UnrealBlueprintClass::UnrealBlueprintClass()
	{
		bpv__Test_Bool_38__pf = true;
		bpv__Test_Float_39__pf = 5.423000f;
		bpv__Test_Int_40__pf = 12;
	}

	void UnrealBlueprintClass::OnCreate()
	{
		bpf__On_Create_33__pf();
		bpf__On_Create_41__pf();
	}

	void UnrealBlueprintClass::OnDestroy()
	{
	}

	void UnrealBlueprintClass::OnUpdate(Timestep ts)
	{
		bpf__On_Update_35__pf(ts.GetSeconds());
	}

	void UnrealBlueprintClass::bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_53(int32_t bpp__EntryPoint__pf)
	{
		
		check(bpp__EntryPoint__pf == 33);
		return; // Termination end of function
}
	void UnrealBlueprintClass::bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_54(int32_t bpp__EntryPoint__pf)
	{
		
		check(bpp__EntryPoint__pf == 35);
		return; // Termination end of function
}
	void UnrealBlueprintClass::bpf__On_Create_33__pf()
	{
		bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_53(33);
	}

	void UnrealBlueprintClass::bpf__On_Update_35__pf(float Delta_Seconds)
	{
		bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_54(35);
	}

	void UnrealBlueprintClass::bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_55(int32_t bpp__EntryPoint__pf)
	{
		
		check(bpp__EntryPoint__pf == 41);
		// Node_bpf__On_Create_41__pf
		// Node_bpf__Branch_43__pf
		if (!bpv__Test_Bool_38__pf)
		{
			return; // Termination end of function
		}
		return; // Termination end of function
}
	void UnrealBlueprintClass::bpf__On_Create_41__pf()
	{
		bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_55(41);
	}

}
