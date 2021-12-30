//Dymatic C++ Node Script - Source - V1.2.2
#include "UnrealBlueprintClass.h"
#include "CustomNodeLibrary.h"

namespace Dymatic {

	UnrealBlueprintClass::UnrealBlueprintClass()
	{
		bpv__Test_Bool_89__pf = true;
		bpv__Test_Float_90__pf = 5.423000f;
		bpv__Test_Int_91__pf = 12;
		bpv__Text_250__pf = "";
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
	}

	void UnrealBlueprintClass::bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_255(int32_t bpp__EntryPoint__pf)
	{
		
		check(bpp__EntryPoint__pf == 84);
		return; // Termination end of function
}
	void UnrealBlueprintClass::bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_256(int32_t bpp__EntryPoint__pf)
	{
		std::string b0l__CallFunc_SomeFunFunction_92_Return_Value__pf{};
		float b0l__CallFunc_Make_Float_254_Return_Value__pf{};
		float b0l__CallFunc_Float4Addition_105_Return_Value__pf{};
		glm::vec4 b0l__CallFunc_Make_Color_120_Return_Value__pf{};
		float b0l__CallFunc_Break_Color_126_r__pf{};
		float b0l__CallFunc_Break_Color_126_g__pf{};
		float b0l__CallFunc_Break_Color_126_b__pf{};
		float b0l__CallFunc_Break_Color_126_a__pf{};
		float b0l__CallFunc_Break_Color_275_r__pf{};
		float b0l__CallFunc_Break_Color_275_g__pf{};
		float b0l__CallFunc_Break_Color_275_b__pf{};
		float b0l__CallFunc_Break_Color_275_a__pf{};
		bool b0l__CallFunc_Break_Float_140_bit_a__pf{};
		bool b0l__CallFunc_Break_Float_140_bit_b__pf{};
		bool b0l__CallFunc_Break_Float_140_bit_c__pf{};
		bool b0l__CallFunc_Break_Float_140_bit_d__pf{};
		bool b0l__CallFunc_Break_Float_268_bit_a__pf{};
		bool b0l__CallFunc_Break_Float_268_bit_b__pf{};
		bool b0l__CallFunc_Break_Float_268_bit_c__pf{};
		bool b0l__CallFunc_Break_Float_268_bit_d__pf{};
		bool b0l__CallFunc_SomeFunFunction_92_Success__pf{};
		float b0l__CallFunc_Make_Float_261_Return_Value__pf{};
		glm::vec4 b0l__CallFunc_Make_Color_152_Return_Value__pf{};
		float b0l__CallFunc_Make_Float_282_Return_Value__pf{};
		bool b0l__CallFunc_BooleanAND_166_Return_Value__pf{};
		float b0l__CallFunc_Break_Color_296_r__pf{};
		float b0l__CallFunc_Break_Color_296_g__pf{};
		float b0l__CallFunc_Break_Color_296_b__pf{};
		float b0l__CallFunc_Break_Color_296_a__pf{};
		bool b0l__CallFunc_Break_Float_289_bit_a__pf{};
		bool b0l__CallFunc_Break_Float_289_bit_b__pf{};
		bool b0l__CallFunc_Break_Float_289_bit_c__pf{};
		bool b0l__CallFunc_Break_Float_289_bit_d__pf{};
		float b0l__CallFunc_FloatAddition_172_Return_Value__pf{};
		bool b0l__CallFunc_Break_Float_303_bit_a__pf{};
		bool b0l__CallFunc_Break_Float_303_bit_b__pf{};
		bool b0l__CallFunc_Break_Float_303_bit_c__pf{};
		bool b0l__CallFunc_Break_Float_303_bit_d__pf{};
		
		check(bpp__EntryPoint__pf == 86);
		// Node_bpf__On_Update_86__pf
		// Comment: Executed every frame and provides delta time.
		// Node_bpf__Some_Fun_Function_92__pf
		DYTestNodeLibrary::Break_Color({ 1.000000, 0.000000, 0.000000, 0.000000 }, /*out*/ b0l__CallFunc_Break_Color_126_r__pf, /*out*/ b0l__CallFunc_Break_Color_126_g__pf, /*out*/ b0l__CallFunc_Break_Color_126_b__pf, /*out*/ b0l__CallFunc_Break_Color_126_a__pf);
		b0l__CallFunc_Make_Color_120_Return_Value__pf = DYTestNodeLibrary::Make_Color(b0l__CallFunc_Break_Color_126_r__pf, b0l__CallFunc_Break_Color_126_b__pf, b0l__CallFunc_Break_Color_126_g__pf, b0l__CallFunc_Break_Color_126_a__pf);
		DYTestNodeLibrary::Break_Color(b0l__CallFunc_Make_Color_120_Return_Value__pf, /*out*/ b0l__CallFunc_Break_Color_275_r__pf, /*out*/ b0l__CallFunc_Break_Color_275_g__pf, /*out*/ b0l__CallFunc_Break_Color_275_b__pf, /*out*/ b0l__CallFunc_Break_Color_275_a__pf);
		DYTestNodeLibrary::Break_Float(b0l__CallFunc_Break_Color_275_a__pf, /*out*/ b0l__CallFunc_Break_Float_140_bit_a__pf, /*out*/ b0l__CallFunc_Break_Float_140_bit_b__pf, /*out*/ b0l__CallFunc_Break_Float_140_bit_c__pf, /*out*/ b0l__CallFunc_Break_Float_140_bit_d__pf);
		b0l__CallFunc_Float4Addition_105_Return_Value__pf = DYTestNodeLibrary::Float4Addition(b0l__CallFunc_Break_Color_275_r__pf, b0l__CallFunc_Break_Color_275_g__pf, b0l__CallFunc_Break_Color_275_b__pf, b0l__CallFunc_Break_Color_275_a__pf, b0l__CallFunc_Break_Float_140_bit_a__pf);
		DYTestNodeLibrary::Break_Float(b0l__CallFunc_Float4Addition_105_Return_Value__pf, /*out*/ b0l__CallFunc_Break_Float_268_bit_a__pf, /*out*/ b0l__CallFunc_Break_Float_268_bit_b__pf, /*out*/ b0l__CallFunc_Break_Float_268_bit_c__pf, /*out*/ b0l__CallFunc_Break_Float_268_bit_d__pf);
		b0l__CallFunc_Make_Float_254_Return_Value__pf = DYTestNodeLibrary::Make_Float(b0l__CallFunc_Break_Float_268_bit_c__pf, b0l__CallFunc_Break_Float_268_bit_b__pf, b0l__CallFunc_Break_Float_268_bit_a__pf, b0l__CallFunc_Break_Float_268_bit_d__pf);
		b0l__CallFunc_BooleanAND_166_Return_Value__pf = DYTestNodeLibrary::BooleanAND(false, false);
		b0l__CallFunc_Make_Float_282_Return_Value__pf = DYTestNodeLibrary::Make_Float(b0l__CallFunc_BooleanAND_166_Return_Value__pf, b0l__CallFunc_BooleanAND_166_Return_Value__pf, true, true);
		b0l__CallFunc_Make_Color_152_Return_Value__pf = DYTestNodeLibrary::Make_Color(0.000000f, 0.000000f, 0.000000f, b0l__CallFunc_Make_Float_282_Return_Value__pf);
		DYTestNodeLibrary::Break_Color(b0l__CallFunc_Make_Color_152_Return_Value__pf, /*out*/ b0l__CallFunc_Break_Color_296_r__pf, /*out*/ b0l__CallFunc_Break_Color_296_g__pf, /*out*/ b0l__CallFunc_Break_Color_296_b__pf, /*out*/ b0l__CallFunc_Break_Color_296_a__pf);
		DYTestNodeLibrary::Break_Float(b0l__CallFunc_Break_Color_296_r__pf, /*out*/ b0l__CallFunc_Break_Float_289_bit_a__pf, /*out*/ b0l__CallFunc_Break_Float_289_bit_b__pf, /*out*/ b0l__CallFunc_Break_Float_289_bit_c__pf, /*out*/ b0l__CallFunc_Break_Float_289_bit_d__pf);
		b0l__CallFunc_FloatAddition_172_Return_Value__pf = DYTestNodeLibrary::FloatAddition(b0l__CallFunc_Break_Color_296_g__pf, b0l__CallFunc_Break_Color_296_b__pf);
		DYTestNodeLibrary::Break_Float(b0l__CallFunc_FloatAddition_172_Return_Value__pf, /*out*/ b0l__CallFunc_Break_Float_303_bit_a__pf, /*out*/ b0l__CallFunc_Break_Float_303_bit_b__pf, /*out*/ b0l__CallFunc_Break_Float_303_bit_c__pf, /*out*/ b0l__CallFunc_Break_Float_303_bit_d__pf);
		b0l__CallFunc_Make_Float_261_Return_Value__pf = DYTestNodeLibrary::Make_Float(b0l__CallFunc_Break_Float_289_bit_a__pf, b0l__CallFunc_Break_Float_303_bit_b__pf, b0l__CallFunc_Break_Float_303_bit_c__pf, b0l__CallFunc_Break_Float_289_bit_d__pf);
		b0l__CallFunc_SomeFunFunction_92_Return_Value__pf = DYTestNodeLibrary::SomeFunFunction(b0l__CallFunc_Make_Float_254_Return_Value__pf, bpv__Text_250__pf, /*out*/ b0l__CallFunc_SomeFunFunction_92_Success__pf, b0l__CallFunc_Make_Float_261_Return_Value__pf);
		return; // Termination end of function
}
	void UnrealBlueprintClass::bpf__On_Create_84__pf()
	{
		bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_255(84);
	}

	void UnrealBlueprintClass::bpf__On_Update_86__pf(float Delta_Seconds)
	{
		bpf__ExecuteUbergraph_UnrealBlueprintClass__pf_256(86);
	}

}
