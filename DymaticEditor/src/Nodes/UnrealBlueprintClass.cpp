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
		bpf__On_Create_92__pf();
	}

	void UnrealBlueprintClass::OnDestroy()
	{
	}

	void UnrealBlueprintClass::OnUpdate(Timestep ts)
	{
		bpf__On_Update_86__pf(ts.GetSeconds());
	}

	void UnrealBlueprintClass::bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_101(int32_t bpp__EntryPoint__pf)
	{
		
		check(bpp__EntryPoint__pf == 84);
		return; // Termination end of function
}
	void UnrealBlueprintClass::bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_102(int32_t bpp__EntryPoint__pf)
	{
		
		check(bpp__EntryPoint__pf == 86);
		return; // Termination end of function
}
	void UnrealBlueprintClass::bpf__On_Create_84__pf()
	{
		bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_101(84);
	}

	void UnrealBlueprintClass::bpf__On_Update_86__pf(float Delta_Seconds)
	{
		bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_102(86);
	}

	void UnrealBlueprintClass::bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_103(int32_t bpp__EntryPoint__pf)
	{
		
		check(bpp__EntryPoint__pf == 92);
		// Node_bpf__On_Create_92__pf
		// Node_bpf__Print_String_94__pf
		DYTestNodeLibrary::PrintString("hello world", { 1.000000, 0.000000, 0.000000, 0.000000 });
		return; // Termination end of function
}
	void UnrealBlueprintClass::bpf__On_Create_92__pf()
	{
		bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_103(92);
	}

}
