
#pragma once
//Dymatic C++ Node Script - V1.2.2

#include "DymaticNodeLibrary.h"
#include "Dymatic/Scene/ScriptableEntity.h"

class BlueprintClass : public ScriptableEntity
{
public:
//Variables
bool DymaticNodeVariable_Test_Bool = true;
float DymaticNodeVariable_Test_Float = 5.423000f;
int DymaticNodeVariable_Test_Int = 12;

//Custom Node Library Additions
void DymaticVariable_Get_Test_Bool(FunctionIn functionIn, FunctionReturn& functionOut)
{functionIn.CalculationFunction();
FunctionReturn returnVal;
returnVal.PinValues.push_back({});
returnVal.PinValues[0].Bool = DymaticNodeVariable_Test_Bool;
functionOut = returnVal;
}

void DymaticVariable_Set_Test_Bool(FunctionIn functionIn, FunctionReturn& functionOut)
{functionIn.CalculationFunction();
FunctionReturn returnVal;
returnVal.PinValues.push_back({});
returnVal.PinValues.push_back({});
DymaticNodeVariable_Test_Bool = functionIn.PinValues[1].Bool;
returnVal.PinValues[1].Bool = DymaticNodeVariable_Test_Bool;
if (!functionIn.ExecutableOutputs.empty()) functionIn.ExecutableOutputs[0]();functionOut = returnVal;
}

void DymaticVariable_Get_Test_Float(FunctionIn functionIn, FunctionReturn& functionOut)
{functionIn.CalculationFunction();
FunctionReturn returnVal;
returnVal.PinValues.push_back({});
returnVal.PinValues[0].Float = DymaticNodeVariable_Test_Float;
functionOut = returnVal;
}

void DymaticVariable_Set_Test_Float(FunctionIn functionIn, FunctionReturn& functionOut)
{functionIn.CalculationFunction();
FunctionReturn returnVal;
returnVal.PinValues.push_back({});
returnVal.PinValues.push_back({});
DymaticNodeVariable_Test_Float = functionIn.PinValues[1].Float;
returnVal.PinValues[1].Float = DymaticNodeVariable_Test_Float;
if (!functionIn.ExecutableOutputs.empty()) functionIn.ExecutableOutputs[0]();functionOut = returnVal;
}

void DymaticVariable_Get_Test_Int(FunctionIn functionIn, FunctionReturn& functionOut)
{functionIn.CalculationFunction();
FunctionReturn returnVal;
returnVal.PinValues.push_back({});
returnVal.PinValues[0].Int = DymaticNodeVariable_Test_Int;
functionOut = returnVal;
}

void DymaticVariable_Set_Test_Int(FunctionIn functionIn, FunctionReturn& functionOut)
{functionIn.CalculationFunction();
FunctionReturn returnVal;
returnVal.PinValues.push_back({});
returnVal.PinValues.push_back({});
DymaticNodeVariable_Test_Int = functionIn.PinValues[1].Int;
returnVal.PinValues[1].Int = DymaticNodeVariable_Test_Int;
if (!functionIn.ExecutableOutputs.empty()) functionIn.ExecutableOutputs[0]();functionOut = returnVal;
}


//Function Save States
FunctionReturn s_NodeEditorFunctionSave_On_Create_1;
FunctionReturn s_NodeEditorFunctionSave_On_Update_3;
FunctionReturn NodeEvent_On_Create_1 (int executableIndex);

FunctionReturn NodeEvent_On_Update_3 (int executableIndex);

virtual void OnCreate() override
{
NodeEvent_On_Create_1(0);
}

virtual void OnUpdate(Timestep ts) override
{
NodeEvent_On_Update_3(0);
}

};

//Executed when script starts.
FunctionReturn BlueprintClass::NodeEvent_On_Create_1 (int executableIndex)
{
s_NodeEditorFunctionSave_On_Create_1.PinValues.clear();
FunctionIn in = { executableIndex, s_NodeEditorFunctionSave_On_Create_1.PinValues, {}, [this]() {  } };
On_Create(in, { s_NodeEditorFunctionSave_On_Create_1 } );
return s_NodeEditorFunctionSave_On_Create_1;

}
//Executed every frame and provides delta time.
FunctionReturn BlueprintClass::NodeEvent_On_Update_3 (int executableIndex)
{
s_NodeEditorFunctionSave_On_Update_3.PinValues.clear();
FunctionIn in = { executableIndex, s_NodeEditorFunctionSave_On_Update_3.PinValues, {}, [this]() {  } };
On_Update(in, { s_NodeEditorFunctionSave_On_Update_3 } );
return s_NodeEditorFunctionSave_On_Update_3;

}
