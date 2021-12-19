//Dymatic C++ Node Script - Source - V1.2.2
#include "UnrealBlueprintClass.h"
#include "CustomNodeLibrary.h"

namespace Dymatic {

	UnrealBlueprintClass::UnrealBlueprintClass()
	{
		bpv__Test_Bool_30__pf = true;
		bpv__Test_Float_31__pf = 5.423000f;
		bpv__Test_Int_32__pf = 12;
	}

	void UnrealBlueprintClass::OnCreate()
	{
		bpf__On_Create_25__pf();
	}

	void UnrealBlueprintClass::OnDestroy()
	{
		bpf__On_Destroy_33__pf();
	}

	void UnrealBlueprintClass::OnUpdate(Timestep ts)
	{
		bpf__On_Update_27__pf(ts.GetSeconds());
	}

	void UnrealBlueprintClass::bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_54(int32_t bpp__EntryPoint__pf)
	{
		
		check(bpp__EntryPoint__pf == 25);
		// Node_bpf__On_Create_25__pf
		// Node_bpf__Print_String_35__pf
		DYTestNodeLibrary::PrintString("On Create", { 0.000000, 0.000000, 0.000000, 0.000000 });
		return; // Termination end of function
}
	void UnrealBlueprintClass::bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_55(int32_t bpp__EntryPoint__pf)
	{
		
		check(bpp__EntryPoint__pf == 27);
		// Node_bpf__On_Update_27__pf
		// Node_bpf__Print_String_41__pf
		DYTestNodeLibrary::PrintString("On Update", { 0.000000, 0.000000, 0.000000, 0.000000 });
		return; // Termination end of function
}
	void UnrealBlueprintClass::bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_56(int32_t bpp__EntryPoint__pf)
	{
		
		check(bpp__EntryPoint__pf == 33);
		// Node_bpf__On_Destroy_33__pf
		// Node_bpf__Print_String_47__pf
		DYTestNodeLibrary::PrintString("On Destroy", { 0.000000, 0.000000, 0.000000, 0.000000 });
		return; // Termination end of function
}
	void UnrealBlueprintClass::bpf__On_Create_25__pf()
	{
		bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_54(25);
	}

	void UnrealBlueprintClass::bpf__On_Update_27__pf(float Delta_Seconds)
	{
		bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_55(27);
	}

	void UnrealBlueprintClass::bpf__On_Destroy_33__pf()
	{
		bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_56(33);
	}

}
