//Dymatic C++ Include Script#include "DymaticNodeLibrary.h"public:FunctionReturn s_NodeEditorFunctionSave_InputAction_Fire_1;FunctionReturn s_NodeEditorFunctionSave_Branch_5;FunctionReturn s_NodeEditorFunctionSave_Do_N_10;FunctionReturn s_NodeEditorFunctionSave_OutputAction_16;FunctionReturn s_NodeEditorFunctionSave_Set_Timer_20;FunctionReturn s_NodeEditorFunctionSave_Sequence_27;FunctionReturn s_NodeEditorFunctionSave_Move_To_30;FunctionReturn s_NodeEditorFunctionSave_Random_Wait_32;FunctionReturn s_NodeEditorFunctionSave_Test_Comment_34;FunctionReturn s_NodeEditorFunctionSave_Test_Comment_35;FunctionReturn s_NodeEditorFunctionSave_<_36;FunctionReturn s_NodeEditorFunctionSave_o.O_40;FunctionReturn s_NodeEditorFunctionSave__44;FunctionReturn s_NodeEditorFunctionSave_Print_String_46;FunctionReturn s_NodeEditorFunctionSave_Transform_50;FunctionReturn s_NodeEditorFunctionSave_Group_53;FunctionReturn s_NodeEditorFunctionSave_On_Create_60;FunctionReturn s_NodeEditorFunctionSave_Make_Literal_Bool_63;FunctionReturn s_NodeEditorFunctionSave_Float_To_String_71;virtual void OnCreate() override{switch ((s_NodeEditorFunctionSave_On_Create_60 = On_Create(s_NodeEditorFunctionSave_On_Create_60)).Executable){case 0: {s_NodeEditorFunctionSave_Print_String_46.PinValues.push_back({});s_NodeEditorFunctionSave_Print_String_46.PinValues.push_back({});s_NodeEditorFunctionSave_Float_To_String_71.PinValues.push_back({});s_NodeEditorFunctionSave_Make_Literal_Bool_63.PinValues.push_back({});s_NodeEditorFunctionSave_Make_Literal_Bool_63.PinValues[0].Float = 0.000000;s_NodeEditorFunctionSave_Float_To_String_71.PinValues[0].Float = Make_Literal_Bool(s_NodeEditorFunctionSave_Make_Literal_Bool_63).PinValues[0].Float;s_NodeEditorFunctionSave_Print_String_46.PinValues[1].String = Float_To_String(s_NodeEditorFunctionSave_Float_To_String_71).PinValues[0].String;switch ((s_NodeEditorFunctionSave_Print_String_46 = Print_String(s_NodeEditorFunctionSave_Print_String_46)).Executable){};break;}};}