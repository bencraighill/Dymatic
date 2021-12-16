#pragma once
#include <string>
#include <vector>
#include "Dymatic/Core/Base.h"
#include "Dymatic/Math/Math.h"
#include "Dymatic/Core/Application.h"

struct PinValueData
{
	bool Array;
	
	bool Bool;
	int Int;
	float Float;
	std::string String;
	glm::vec3 Vector;
};

struct FunctionIn;
struct FunctionReturn;

struct FunctionIn
{
	int Executable = 0;
	std::vector<PinValueData>& PinValues;
	std::vector<std::function<void()>> ExecutableOutputs;
	std::function<void()> CalculationFunction;
};


struct FunctionReturn
{
	std::vector<PinValueData> PinValues;
	void* data = nullptr;
};

// Development
void Assert(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	DY_ASSERT(functionIn.PinValues[1].Bool, functionIn.PinValues[2].String);

	if (!functionIn.ExecutableOutputs.empty())
		functionIn.ExecutableOutputs[0]();
}

void Error(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	DY_ERROR(functionIn.PinValues[1].String);

	if (!functionIn.ExecutableOutputs.empty())
		functionIn.ExecutableOutputs[0]();
}

// Events
void On_Create(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();
}

void On_Update(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues.push_back({});
	returnVal.PinValues[1].Float = Dymatic::Application::Get().GetTimestep().GetSeconds();

	functionOut = returnVal;

	if (!functionIn.ExecutableOutputs.empty())
		functionIn.ExecutableOutputs[0]();
}

// Variables
// { Handled in Client Code at Compile Time }

// Math -> Boolean
void Make_Literal_Bool(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Bool = functionIn.PinValues[0].Bool;
	functionOut = returnVal;
}

void AND_Boolean(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Bool = (functionIn.PinValues[0].Bool && functionIn.PinValues[1].Bool);
	functionOut = returnVal;
}

void Equal_Boolean(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Bool = (functionIn.PinValues[0].Bool == functionIn.PinValues[1].Bool);
	functionOut = returnVal;
}

void NAND_Boolean(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Bool = !(functionIn.PinValues[0].Bool && functionIn.PinValues[1].Bool);
	functionOut = returnVal;
}

void NOR_Boolean(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Bool = !(functionIn.PinValues[0].Bool || functionIn.PinValues[1].Bool);
	functionOut = returnVal;
}

void NOT_Boolean(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Bool = !(functionIn.PinValues[0].Bool);
	functionOut = returnVal;
}

void Not_Equal_Boolean(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Bool = !(functionIn.PinValues[0].Bool == functionIn.PinValues[1].Bool);
	functionOut = returnVal;
}

void OR_Boolean(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Bool = (functionIn.PinValues[0].Bool || functionIn.PinValues[1].Bool);
	functionOut = returnVal;
}

void XOR_Boolean(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Bool = ((functionIn.PinValues[0].Bool || functionIn.PinValues[1].Bool) && !(functionIn.PinValues[0].Bool && functionIn.PinValues[1].Bool));
	functionOut = returnVal;
}

// Math -> Int
void Make_Literal_Int(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Int = functionIn.PinValues[0].Int;
	functionOut = returnVal;
}

// Math -> Float
void Make_Literal_Float(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Float = functionIn.PinValues[0].Float;
	functionOut = returnVal;
}

void Modulo_Float(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Float = std::fmod(functionIn.PinValues[0].Float, functionIn.PinValues[1].Float);
	functionOut = returnVal;
}

void Absolute_Float(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Float = Dymatic::Math::Absolute(functionIn.PinValues[0].Float);
	functionOut = returnVal;
}

void Ceil(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Int = std::ceil(functionIn.PinValues[0].Float);
	functionOut = returnVal;
}

void Clamp_Float(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Float = std::clamp(functionIn.PinValues[0].Float, functionIn.PinValues[1].Float, functionIn.PinValues[2].Float);
	functionOut = returnVal;
}

void Compare_Float(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	if (functionIn.PinValues[1].Float > functionIn.PinValues[2].Float && functionIn.ExecutableOutputs.size() > 0)
		functionIn.ExecutableOutputs[0]();
	else if (functionIn.PinValues[1].Float < functionIn.PinValues[2].Float && functionIn.ExecutableOutputs.size() > 2)
		functionIn.ExecutableOutputs[2]();
	else if (functionIn.ExecutableOutputs.size() > 1)
		functionIn.ExecutableOutputs[1]();
}

void Normalize_Angle(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Float = Dymatic::Math::NormalizeAngle(functionIn.PinValues[0].Float, /*-180*/functionIn.PinValues[1].Float, /*360.0f*/ std::abs(functionIn.PinValues[2].Float - functionIn.PinValues[1].Float));
	functionOut = returnVal;
}

void Lerp_Angle(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Float = Dymatic::Math::LerpAngle(functionIn.PinValues[0].Float, functionIn.PinValues[1].Float, functionIn.PinValues[2].Float, functionIn.PinValues[3].Float, functionIn.PinValues[4].Float);
	functionOut = returnVal;
}

void Equal_Float(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Bool = functionIn.PinValues[0].Float == functionIn.PinValues[1].Float;
	functionOut = returnVal;
}

void Exp_Float(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Float = std::exp(functionIn.PinValues[0].Float);
	functionOut = returnVal;
}

void FInterp_Ease_in_Out(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Float = Dymatic::Math::InterpEaseInOut(functionIn.PinValues[0].Float, functionIn.PinValues[1].Float, functionIn.PinValues[2].Float, functionIn.PinValues[3].Float);
	functionOut = returnVal;
}

void Float_Subtraction(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Float = functionIn.PinValues[0].Float - functionIn.PinValues[1].Float;
	functionOut = returnVal;
}

void Float_Multiplication(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	float value = 1.0f;
	for (auto& pin : functionIn.PinValues)
		value *= pin.Float;

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Float = value;
	functionOut = returnVal;
}

void Float_Division(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Float = functionIn.PinValues[0].Float / functionIn.PinValues[1].Float;
	functionOut = returnVal;
}

void Float_Addition(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	float value = 0.0f;
	for (auto& pin : functionIn.PinValues)
		value += pin.Float;

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Float = value;
	functionOut = returnVal;
}

// Math -> Interpolation
void Interp_Ease_In(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Float = Dymatic::Math::InterpEaseIn(functionIn.PinValues[0].Float, functionIn.PinValues[1].Float, functionIn.PinValues[2].Float, functionIn.PinValues[3].Float);
	functionOut = returnVal;
}

void Interp_Ease_Out(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Float = Dymatic::Math::InterpEaseOut(functionIn.PinValues[0].Float, functionIn.PinValues[1].Float, functionIn.PinValues[2].Float, functionIn.PinValues[3].Float);
	functionOut = returnVal;
}

// Math -> Trig

// Math -> Conversions
void Int_To_Bool(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].Bool = functionIn.PinValues[0].Int;

	functionOut = returnVal;
}

void Float_To_Bool(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].Bool = functionIn.PinValues[0].Float;

	functionOut = returnVal;
}

void String_To_Bool(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].Bool = (functionIn.PinValues[0].String == "true" || functionIn.PinValues[0].String == "True") ? true : false;
	functionOut = returnVal;
}

void Bool_To_Int(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].Int = functionIn.PinValues[0].Bool ? 1 : 0;

	functionOut = returnVal;
}

void Float_To_Int(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].Int = functionIn.PinValues[0].Float;

	functionOut = returnVal;
}

void String_To_Int(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	std::string IntString = functionIn.PinValues[0].String;


	for (int i = 0; i < IntString.length(); i++)
	{
		if (!isdigit(IntString[i]))
		{
			IntString.erase(i, 1);
			i--;
		}
	}

	returnVal.PinValues[0].Int = std::stof(IntString);
	functionOut = returnVal;
}

void Bool_To_Float(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].Float = functionIn.PinValues[0].Bool ? 1.0f : 0.0f;

	functionOut = returnVal;
}

void Int_To_Float(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].Float = functionIn.PinValues[0].Int;

	functionOut = returnVal;
}

void String_To_Float(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	std::string IntString = functionIn.PinValues[0].String;


	for (int i = 0; i < IntString.length(); i++)
	{
		if (!isdigit(IntString[i]))
		{
			IntString.erase(i, 1);
			i--;
		}
	}

	returnVal.PinValues[0].Float = std::stof(IntString);
	functionOut = returnVal;
}

void Float_To_Vector(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();
	
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	
	returnVal.PinValues[0].Vector = glm::vec3(functionIn.PinValues[0].Float);
	functionOut = returnVal;
}

void String_To_Vector(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();
	
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	
	auto string = functionIn.PinValues[0].String;
	auto& vector = returnVal.PinValues[0].Vector;

	// Remove all spaces from string (string is copy)
	string.erase(remove_if(string.begin(), string.end(), isspace), string.end());

	if (string.find("X=") != std::string::npos)
	{
		std::string number = "";
		for (int i = string.find("X=") + 2; i < string.length(); i++)
		{
			auto& character = string[i];
			if (isdigit(character) || character == '.')
				number += character;
			else
				break;
		}
		if (!number.empty())
			vector.x = std::stof(number);
	}
	if (string.find("Y=") != std::string::npos)
	{
		std::string number = "";
		for (int i = string.find("Y=") + 2; i < string.length(); i++)
		{
			auto& character = string[i];
			if (isdigit(character) || character == '.')
				number += character;
			else
				break;
		}
		if (!number.empty())
			vector.y = std::stof(number);
	}
	if (string.find("Z=") != std::string::npos)
	{
		std::string number = "";
		for (int i = string.find("Z=") + 2; i < string.length(); i++)
		{
			auto& character = string[i];
			if (isdigit(character) || character == '.')
				number += character;
			else
				break;
		}
		if (!number.empty())
			vector.z = std::stof(number);
	}

	functionOut = returnVal;
}

// Utilities -> String
void Make_Literal_String(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].String = functionIn.PinValues[0].String;
	functionOut = returnVal;
}

void Bool_To_String(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].String = functionIn.PinValues[0].Bool ? "true" : "false";

	functionOut = returnVal;
}

void Int_To_String(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].String = std::to_string(functionIn.PinValues[0].Int);

	functionOut = returnVal;
}

void Float_To_String(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].String = std::to_string(functionIn.PinValues[0].Float);

	functionOut = returnVal;
}

void Vector_To_String(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();
	
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	
	returnVal.PinValues[0].String = "X=" + std::to_string(functionIn.PinValues[0].Vector.x) + " Y=" + std::to_string(functionIn.PinValues[0].Vector.y) + " Z=" + std::to_string(functionIn.PinValues[0].Vector.z);
	
	functionOut = returnVal;
}

void Print_String(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	DY_TRACE(functionIn.PinValues[1].String);

	if (!functionIn.ExecutableOutputs.empty())
		functionIn.ExecutableOutputs[0]();
}

// Utilities -> Flow Control
void Branch(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	int index = functionIn.PinValues[1].Bool ? 0 : 1;
	if (index < functionIn.ExecutableOutputs.size())
		functionIn.ExecutableOutputs[index]();

}

void Do_N(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	switch (functionIn.Executable)
	{
	case 0: {
		if ((int)functionOut.data < functionIn.PinValues[1].Int)
		{
			FunctionReturn returnVal;
			returnVal.PinValues.push_back({});
			returnVal.PinValues.push_back({});
			returnVal.data = (void*)((int)functionOut.data + 1);
			returnVal.PinValues[1].Int = (int)returnVal.data;
			functionOut = returnVal;

			if (!functionIn.ExecutableOutputs.empty())
				functionIn.ExecutableOutputs[0]();
		}
		break; }
	case 1: { functionOut.data = nullptr; break; }
	}
}

void Do_Once(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	if (functionOut.data == nullptr)
		functionOut.data = (void*)(functionIn.PinValues[2].Bool ? 2 : 1);

	switch (functionIn.Executable)
	{
	case 0: {
		if (functionOut.data == (void*)1)
		{
			functionOut.data = (void*)2;

			if (!functionIn.ExecutableOutputs.empty())
				functionIn.ExecutableOutputs[0]();
		}
		break; }
	case 1: { functionOut.data = (void*)1; break; }
	}
}

void For_Loop(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	for (int i = functionIn.PinValues[1].Int; i <= functionIn.PinValues[2].Int; i++, functionIn.CalculationFunction())
	{
		functionOut.PinValues[1].Int = i;

		if (!functionIn.ExecutableOutputs.empty())
		{
			functionIn.ExecutableOutputs[0]();
		}
	}

	if (functionIn.ExecutableOutputs.size() > 1)
		functionIn.ExecutableOutputs[1]();
}

void For_Loop_with_Break(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	switch (functionIn.Executable)
	{
	case 0:
	{
		functionOut.data = nullptr;
		for (int i = functionIn.PinValues[1].Int; i <= functionIn.PinValues[2].Int; i++, functionIn.CalculationFunction())
		{
			if (functionOut.data == (void*)(1)) break;

			functionOut.PinValues[1].Int = i;

			if (!functionIn.ExecutableOutputs.empty())
			{
				functionIn.ExecutableOutputs[0]();
			}
		}
		break;
	}
	case 1:
	{
		functionOut.data = (void*)(1);
		break;
	}
	}

	if (functionIn.ExecutableOutputs.size() > 1)
		functionIn.ExecutableOutputs[1]();
}

void Sequence(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	if (!functionIn.ExecutableOutputs.empty())
		for (auto& func : functionIn.ExecutableOutputs)
			func();
}

void While_Loop(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	while (functionIn.PinValues[1].Bool)
	{
		if (!functionIn.ExecutableOutputs.empty())
			functionIn.ExecutableOutputs[0]();

		functionIn.CalculationFunction();
	}

	if (functionIn.ExecutableOutputs.size() > 1)
		functionIn.ExecutableOutputs[1]();
}

// No Category
// { Comment }

// Unimplimented

void Random_Bool(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].Bool = Dymatic::Math::GetRandomInRange(0, 2) == 1;
	functionOut = returnVal;
}

void Random_Float(FunctionIn functionIn, FunctionReturn& functionOut)
{
	functionIn.CalculationFunction();

	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].Float = Dymatic::Math::GetRandomInRange(0, 9999999);
	functionOut = returnVal;
}