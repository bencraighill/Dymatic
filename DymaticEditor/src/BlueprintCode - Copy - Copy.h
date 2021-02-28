#pragma once
//Dymatic C++ Include Script

#include "DymaticNodeLibrary.h"
#include "Dymatic/Scene/ScriptableEntity.h"

class BlueprintClass : public ScriptableEntity
{
public:
FunctionReturn s_NodeEditorFunctionSave_On_Update_70;
FunctionReturn s_NodeEditorFunctionSave_On_Update_79;
FunctionReturn s_NodeEditorFunctionSave_On_Create_82;
FunctionReturn s_NodeEditorFunctionSave_On_Create_84;
FunctionReturn s_NodeEditorFunctionSave_On_Create_86;
FunctionReturn s_NodeEditorFunctionSave_On_Create_88;
FunctionReturn s_NodeEditorFunctionSave_Sequence_90;
FunctionReturn s_NodeEditorFunctionSave_Print_String_96;
FunctionReturn s_NodeEditorFunctionSave_Print_String_101;
FunctionReturn s_NodeEditorFunctionSave_Sequence_106;
FunctionReturn s_NodeEditorFunctionSave_Print_String_112;
FunctionReturn s_NodeEditorFunctionSave_Print_String_117;
FunctionReturn s_NodeEditorFunctionSave_Sequence_122;
FunctionReturn s_NodeEditorFunctionSave_Print_String_130;
FunctionReturn s_NodeEditorFunctionSave_Print_String_135;
FunctionReturn s_NodeEditorFunctionSave_Print_String_140;
FunctionReturn s_NodeEditorFunctionSave_Sequence_145;
FunctionReturn s_NodeEditorFunctionSave_Print_String_151;
FunctionReturn s_NodeEditorFunctionSave_Print_String_156;
FunctionReturn s_NodeEditorFunctionSave_Sequence_161;
FunctionReturn s_NodeEditorFunctionSave_Print_String_167;
FunctionReturn s_NodeEditorFunctionSave_Print_String_172;
FunctionReturn s_NodeEditorFunctionSave_On_Update_177;
FunctionReturn s_NodeEditorFunctionSave_Sequence_180;
FunctionReturn s_NodeEditorFunctionSave_Print_String_186;
FunctionReturn s_NodeEditorFunctionSave_Print_String_200;
FunctionReturn s_NodeEditorFunctionSave_On_Update_205;
FunctionReturn s_NodeEditorFunctionSave_Sequence_213;
FunctionReturn s_NodeEditorFunctionSave_Print_String_222;
FunctionReturn s_NodeEditorFunctionSave_Print_String_227;
FunctionReturn s_NodeEditorFunctionSave_Sequence_242;
FunctionReturn s_NodeEditorFunctionSave_Print_String_251;
FunctionReturn s_NodeEditorFunctionSave_Float_To_String_259;
FunctionReturn NodeEvent_On_Update_70 ();

FunctionReturn NodeEvent_On_Update_79 ();

FunctionReturn NodeEvent_On_Create_82 ();

FunctionReturn NodeEvent_On_Create_84 ();

FunctionReturn NodeEvent_On_Create_86 ();

FunctionReturn NodeEvent_On_Create_88 ();

FunctionReturn NodeEvent_Sequence_90 ();

FunctionReturn NodeEvent_Print_String_96 ();

FunctionReturn NodeEvent_Print_String_101 ();

FunctionReturn NodeEvent_Sequence_106 ();

FunctionReturn NodeEvent_Print_String_112 ();

FunctionReturn NodeEvent_Print_String_117 ();

FunctionReturn NodeEvent_Sequence_122 ();

FunctionReturn NodeEvent_Print_String_130 ();

FunctionReturn NodeEvent_Print_String_135 ();

FunctionReturn NodeEvent_Print_String_140 ();

FunctionReturn NodeEvent_Sequence_145 ();

FunctionReturn NodeEvent_Print_String_151 ();

FunctionReturn NodeEvent_Print_String_156 ();

FunctionReturn NodeEvent_Sequence_161 ();

FunctionReturn NodeEvent_Print_String_167 ();

FunctionReturn NodeEvent_Print_String_172 ();

FunctionReturn NodeEvent_On_Update_177 ();

FunctionReturn NodeEvent_Sequence_180 ();

FunctionReturn NodeEvent_Print_String_186 ();

FunctionReturn NodeEvent_Print_String_200 ();

FunctionReturn NodeEvent_On_Update_205 ();

FunctionReturn NodeEvent_Sequence_213 ();

FunctionReturn NodeEvent_Print_String_222 ();

FunctionReturn NodeEvent_Print_String_227 ();

FunctionReturn NodeEvent_Sequence_242 ();

FunctionReturn NodeEvent_Print_String_251 ();

FunctionReturn NodeEvent_Float_To_String_259 ();

virtual void OnCreate() override
{
NodeEvent_On_Create_82();
NodeEvent_On_Create_84();
NodeEvent_On_Create_86();
NodeEvent_On_Create_88();
}

virtual void OnUpdate(Timestep ts) override
{
NodeEvent_On_Update_70();
NodeEvent_On_Update_79();
NodeEvent_On_Update_177();
NodeEvent_On_Update_205();
}

};

FunctionReturn BlueprintClass::NodeEvent_On_Update_70 ()
{
for (int index : (s_NodeEditorFunctionSave_On_Update_70 = On_Update(s_NodeEditorFunctionSave_On_Update_70)).Executable){switch (index){
case 0: { NodeEvent_Sequence_145(); break; }
};
};

return s_NodeEditorFunctionSave_On_Update_70;

}
FunctionReturn BlueprintClass::NodeEvent_On_Update_79 ()
{
for (int index : (s_NodeEditorFunctionSave_On_Update_79 = On_Update(s_NodeEditorFunctionSave_On_Update_79)).Executable){switch (index){
case 0: { NodeEvent_Sequence_161(); break; }
};
};

return s_NodeEditorFunctionSave_On_Update_79;

}
FunctionReturn BlueprintClass::NodeEvent_On_Create_82 ()
{
for (int index : (s_NodeEditorFunctionSave_On_Create_82 = On_Create(s_NodeEditorFunctionSave_On_Create_82)).Executable){switch (index){
case 0: { NodeEvent_Sequence_122(); break; }
};
};

return s_NodeEditorFunctionSave_On_Create_82;

}
FunctionReturn BlueprintClass::NodeEvent_On_Create_84 ()
{
for (int index : (s_NodeEditorFunctionSave_On_Create_84 = On_Create(s_NodeEditorFunctionSave_On_Create_84)).Executable){switch (index){
case 0: { NodeEvent_Sequence_122(); break; }
};
};

return s_NodeEditorFunctionSave_On_Create_84;

}
FunctionReturn BlueprintClass::NodeEvent_On_Create_86 ()
{
for (int index : (s_NodeEditorFunctionSave_On_Create_86 = On_Create(s_NodeEditorFunctionSave_On_Create_86)).Executable){switch (index){
case 0: { NodeEvent_Sequence_106(); break; }
};
};

return s_NodeEditorFunctionSave_On_Create_86;

}
FunctionReturn BlueprintClass::NodeEvent_On_Create_88 ()
{
for (int index : (s_NodeEditorFunctionSave_On_Create_88 = On_Create(s_NodeEditorFunctionSave_On_Create_88)).Executable){switch (index){
case 0: { NodeEvent_Sequence_90(); break; }
};
};

return s_NodeEditorFunctionSave_On_Create_88;

}
FunctionReturn BlueprintClass::NodeEvent_Sequence_90 ()
{
s_NodeEditorFunctionSave_Sequence_90.PinValues.push_back({});
s_NodeEditorFunctionSave_Sequence_90.PinValues.push_back({});
s_NodeEditorFunctionSave_Sequence_90.PinValues[1].Int = 1;
for (int index : (s_NodeEditorFunctionSave_Sequence_90 = Sequence(s_NodeEditorFunctionSave_Sequence_90)).Executable){switch (index){
case 0: { NodeEvent_Print_String_96(); break; }
case 1: { NodeEvent_Print_String_101(); break; }
};
};

return s_NodeEditorFunctionSave_Sequence_90;

}
FunctionReturn BlueprintClass::NodeEvent_Print_String_96 ()
{
s_NodeEditorFunctionSave_Print_String_96.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_96.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_96.PinValues[1].String = "1A";
for (int index : (s_NodeEditorFunctionSave_Print_String_96 = Print_String(s_NodeEditorFunctionSave_Print_String_96)).Executable){switch (index){
};
};

return s_NodeEditorFunctionSave_Print_String_96;

}
FunctionReturn BlueprintClass::NodeEvent_Print_String_101 ()
{
s_NodeEditorFunctionSave_Print_String_101.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_101.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_101.PinValues[1].String = "1B";
for (int index : (s_NodeEditorFunctionSave_Print_String_101 = Print_String(s_NodeEditorFunctionSave_Print_String_101)).Executable){switch (index){
};
};

return s_NodeEditorFunctionSave_Print_String_101;

}
FunctionReturn BlueprintClass::NodeEvent_Sequence_106 ()
{
s_NodeEditorFunctionSave_Sequence_106.PinValues.push_back({});
s_NodeEditorFunctionSave_Sequence_106.PinValues.push_back({});
s_NodeEditorFunctionSave_Sequence_106.PinValues[1].Int = 1;
for (int index : (s_NodeEditorFunctionSave_Sequence_106 = Sequence(s_NodeEditorFunctionSave_Sequence_106)).Executable){switch (index){
case 0: { NodeEvent_Print_String_112(); break; }
case 1: { NodeEvent_Print_String_117(); break; }
};
};

return s_NodeEditorFunctionSave_Sequence_106;

}
FunctionReturn BlueprintClass::NodeEvent_Print_String_112 ()
{
s_NodeEditorFunctionSave_Print_String_112.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_112.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_112.PinValues[1].String = "2A";
for (int index : (s_NodeEditorFunctionSave_Print_String_112 = Print_String(s_NodeEditorFunctionSave_Print_String_112)).Executable){switch (index){
};
};

return s_NodeEditorFunctionSave_Print_String_112;

}
FunctionReturn BlueprintClass::NodeEvent_Print_String_117 ()
{
s_NodeEditorFunctionSave_Print_String_117.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_117.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_117.PinValues[1].String = "2B";
for (int index : (s_NodeEditorFunctionSave_Print_String_117 = Print_String(s_NodeEditorFunctionSave_Print_String_117)).Executable){switch (index){
};
};

return s_NodeEditorFunctionSave_Print_String_117;

}
FunctionReturn BlueprintClass::NodeEvent_Sequence_122 ()
{
s_NodeEditorFunctionSave_Sequence_122.PinValues.push_back({});
s_NodeEditorFunctionSave_Sequence_122.PinValues.push_back({});
s_NodeEditorFunctionSave_Sequence_122.PinValues[1].Int = 2;
for (int index : (s_NodeEditorFunctionSave_Sequence_122 = Sequence(s_NodeEditorFunctionSave_Sequence_122)).Executable){switch (index){
case 0: { NodeEvent_Print_String_130(); break; }
case 1: { NodeEvent_Print_String_135(); break; }
case 2: { NodeEvent_Print_String_140(); break; }
};
};

return s_NodeEditorFunctionSave_Sequence_122;

}
FunctionReturn BlueprintClass::NodeEvent_Print_String_130 ()
{
s_NodeEditorFunctionSave_Print_String_130.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_130.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_130.PinValues[1].String = "3A";
for (int index : (s_NodeEditorFunctionSave_Print_String_130 = Print_String(s_NodeEditorFunctionSave_Print_String_130)).Executable){switch (index){
};
};

return s_NodeEditorFunctionSave_Print_String_130;

}
FunctionReturn BlueprintClass::NodeEvent_Print_String_135 ()
{
s_NodeEditorFunctionSave_Print_String_135.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_135.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_135.PinValues[1].String = "3B";
for (int index : (s_NodeEditorFunctionSave_Print_String_135 = Print_String(s_NodeEditorFunctionSave_Print_String_135)).Executable){switch (index){
};
};

return s_NodeEditorFunctionSave_Print_String_135;

}
FunctionReturn BlueprintClass::NodeEvent_Print_String_140 ()
{
s_NodeEditorFunctionSave_Print_String_140.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_140.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_140.PinValues[1].String = "3C";
for (int index : (s_NodeEditorFunctionSave_Print_String_140 = Print_String(s_NodeEditorFunctionSave_Print_String_140)).Executable){switch (index){
};
};

return s_NodeEditorFunctionSave_Print_String_140;

}
FunctionReturn BlueprintClass::NodeEvent_Sequence_145 ()
{
s_NodeEditorFunctionSave_Sequence_145.PinValues.push_back({});
s_NodeEditorFunctionSave_Sequence_145.PinValues.push_back({});
s_NodeEditorFunctionSave_Sequence_145.PinValues[1].Int = 1;
for (int index : (s_NodeEditorFunctionSave_Sequence_145 = Sequence(s_NodeEditorFunctionSave_Sequence_145)).Executable){switch (index){
case 0: { NodeEvent_Print_String_151(); break; }
case 1: { NodeEvent_Print_String_156(); break; }
};
};

return s_NodeEditorFunctionSave_Sequence_145;

}
FunctionReturn BlueprintClass::NodeEvent_Print_String_151 ()
{
s_NodeEditorFunctionSave_Print_String_151.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_151.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_151.PinValues[1].String = "Update 1A";
for (int index : (s_NodeEditorFunctionSave_Print_String_151 = Print_String(s_NodeEditorFunctionSave_Print_String_151)).Executable){switch (index){
};
};

return s_NodeEditorFunctionSave_Print_String_151;

}
FunctionReturn BlueprintClass::NodeEvent_Print_String_156 ()
{
s_NodeEditorFunctionSave_Print_String_156.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_156.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_156.PinValues[1].String = "Update 1B";
for (int index : (s_NodeEditorFunctionSave_Print_String_156 = Print_String(s_NodeEditorFunctionSave_Print_String_156)).Executable){switch (index){
};
};

return s_NodeEditorFunctionSave_Print_String_156;

}
FunctionReturn BlueprintClass::NodeEvent_Sequence_161 ()
{
s_NodeEditorFunctionSave_Sequence_161.PinValues.push_back({});
s_NodeEditorFunctionSave_Sequence_161.PinValues.push_back({});
s_NodeEditorFunctionSave_Sequence_161.PinValues[1].Int = 1;
for (int index : (s_NodeEditorFunctionSave_Sequence_161 = Sequence(s_NodeEditorFunctionSave_Sequence_161)).Executable){switch (index){
case 0: { NodeEvent_Print_String_167(); break; }
case 1: { NodeEvent_Print_String_172(); break; }
};
};

return s_NodeEditorFunctionSave_Sequence_161;

}
FunctionReturn BlueprintClass::NodeEvent_Print_String_167 ()
{
s_NodeEditorFunctionSave_Print_String_167.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_167.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_167.PinValues[1].String = "Update 2A";
for (int index : (s_NodeEditorFunctionSave_Print_String_167 = Print_String(s_NodeEditorFunctionSave_Print_String_167)).Executable){switch (index){
};
};

return s_NodeEditorFunctionSave_Print_String_167;

}
FunctionReturn BlueprintClass::NodeEvent_Print_String_172 ()
{
s_NodeEditorFunctionSave_Print_String_172.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_172.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_172.PinValues[1].String = "Update 2B";
for (int index : (s_NodeEditorFunctionSave_Print_String_172 = Print_String(s_NodeEditorFunctionSave_Print_String_172)).Executable){switch (index){
};
};

return s_NodeEditorFunctionSave_Print_String_172;

}
FunctionReturn BlueprintClass::NodeEvent_On_Update_177 ()
{
for (int index : (s_NodeEditorFunctionSave_On_Update_177 = On_Update(s_NodeEditorFunctionSave_On_Update_177)).Executable){switch (index){
case 0: { NodeEvent_Sequence_180(); break; }
};
};

return s_NodeEditorFunctionSave_On_Update_177;

}
FunctionReturn BlueprintClass::NodeEvent_Sequence_180 ()
{
s_NodeEditorFunctionSave_Sequence_180.PinValues.push_back({});
s_NodeEditorFunctionSave_Sequence_180.PinValues.push_back({});
s_NodeEditorFunctionSave_Sequence_180.PinValues[1].Int = 1;
for (int index : (s_NodeEditorFunctionSave_Sequence_180 = Sequence(s_NodeEditorFunctionSave_Sequence_180)).Executable){switch (index){
case 0: { NodeEvent_Print_String_186(); break; }
case 1: { NodeEvent_Print_String_200(); break; }
};
};

return s_NodeEditorFunctionSave_Sequence_180;

}
FunctionReturn BlueprintClass::NodeEvent_Print_String_186 ()
{
s_NodeEditorFunctionSave_Print_String_186.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_186.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_186.PinValues[1].String = "Update 3A";
for (int index : (s_NodeEditorFunctionSave_Print_String_186 = Print_String(s_NodeEditorFunctionSave_Print_String_186)).Executable){switch (index){
};
};

return s_NodeEditorFunctionSave_Print_String_186;

}
FunctionReturn BlueprintClass::NodeEvent_Print_String_200 ()
{
s_NodeEditorFunctionSave_Print_String_200.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_200.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_200.PinValues[1].String = "Update 3B";
for (int index : (s_NodeEditorFunctionSave_Print_String_200 = Print_String(s_NodeEditorFunctionSave_Print_String_200)).Executable){switch (index){
};
};

return s_NodeEditorFunctionSave_Print_String_200;

}
FunctionReturn BlueprintClass::NodeEvent_On_Update_205 ()
{
for (int index : (s_NodeEditorFunctionSave_On_Update_205 = On_Update(s_NodeEditorFunctionSave_On_Update_205)).Executable){switch (index){
case 0: { NodeEvent_Sequence_213(); break; }
};
};

return s_NodeEditorFunctionSave_On_Update_205;

}
FunctionReturn BlueprintClass::NodeEvent_Sequence_213 ()
{
s_NodeEditorFunctionSave_Sequence_213.PinValues.push_back({});
s_NodeEditorFunctionSave_Sequence_213.PinValues.push_back({});
s_NodeEditorFunctionSave_Sequence_213.PinValues[1].Int = 1;
for (int index : (s_NodeEditorFunctionSave_Sequence_213 = Sequence(s_NodeEditorFunctionSave_Sequence_213)).Executable){switch (index){
case 0: { NodeEvent_Sequence_180(); break; }
case 1: { NodeEvent_Sequence_242(); break; }
};
};

return s_NodeEditorFunctionSave_Sequence_213;

}
FunctionReturn BlueprintClass::NodeEvent_Print_String_222 ()
{
s_NodeEditorFunctionSave_Print_String_222.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_222.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_222.PinValues[1].String = "//------------------DELTA TIME:---------------------\\";
for (int index : (s_NodeEditorFunctionSave_Print_String_222 = Print_String(s_NodeEditorFunctionSave_Print_String_222)).Executable){switch (index){
};
};

return s_NodeEditorFunctionSave_Print_String_222;

}
FunctionReturn BlueprintClass::NodeEvent_Print_String_227 ()
{
s_NodeEditorFunctionSave_Print_String_227.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_227.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_227.PinValues[1].String = "\\---------------------------------------------------------//";
for (int index : (s_NodeEditorFunctionSave_Print_String_227 = Print_String(s_NodeEditorFunctionSave_Print_String_227)).Executable){switch (index){
};
};

return s_NodeEditorFunctionSave_Print_String_227;

}
FunctionReturn BlueprintClass::NodeEvent_Sequence_242 ()
{
s_NodeEditorFunctionSave_Sequence_242.PinValues.push_back({});
s_NodeEditorFunctionSave_Sequence_242.PinValues.push_back({});
s_NodeEditorFunctionSave_Sequence_242.PinValues[1].Int = 2;
for (int index : (s_NodeEditorFunctionSave_Sequence_242 = Sequence(s_NodeEditorFunctionSave_Sequence_242)).Executable){switch (index){
case 0: { NodeEvent_Print_String_222(); break; }
case 1: { NodeEvent_Print_String_227(); break; }
case 2: { NodeEvent_Print_String_251(); break; }
};
};

return s_NodeEditorFunctionSave_Sequence_242;

}
FunctionReturn BlueprintClass::NodeEvent_Print_String_251 ()
{
s_NodeEditorFunctionSave_Print_String_251.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_251.PinValues.push_back({});
s_NodeEditorFunctionSave_Print_String_251.PinValues[1].String = NodeEvent_Float_To_String_259().PinValues[0].String;
for (int index : (s_NodeEditorFunctionSave_Print_String_251 = Print_String(s_NodeEditorFunctionSave_Print_String_251)).Executable){switch (index){
};
};

return s_NodeEditorFunctionSave_Print_String_251;

}
FunctionReturn BlueprintClass::NodeEvent_Float_To_String_259 ()
{
s_NodeEditorFunctionSave_Float_To_String_259.PinValues.push_back({});
s_NodeEditorFunctionSave_Float_To_String_259.PinValues[0].Float = s_NodeEditorFunctionSave_On_Update_205.PinValues[1].Float;
s_NodeEditorFunctionSave_Float_To_String_259 = Float_To_String(s_NodeEditorFunctionSave_Float_To_String_259);
return s_NodeEditorFunctionSave_Float_To_String_259;

}
