#pragma once

#include "Dymatic/Math/Math.h"

struct PinValueData
{
	bool Bool = false;
	int Int = 0;
	float Float = 0.0f;
	std::string String = "";
};

struct FunctionIn
{
	int Executable = 0;
	std::vector<PinValueData> PinValues;
};

struct FunctionReturn
{
	int Executable = 0;
	std::vector<PinValueData> PinValues;

	bool cacheData = false;
};

FunctionReturn On_Create(FunctionReturn functionIn)
{
	return {};
}

FunctionReturn Print_String(FunctionReturn functionIn)
{
	DY_CORE_INFO(functionIn.PinValues[1].String);
	return {};
}

//Operators
//Bool Operators

FunctionReturn AND_Boolean(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Bool = (functionIn.PinValues[0].Bool && functionIn.PinValues[1].Bool);
	return returnVal;
}

FunctionReturn Equal_Boolean(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Bool = (functionIn.PinValues[0].Bool == functionIn.PinValues[1].Bool);
	return returnVal;
}

FunctionReturn NAND_Boolean(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Bool = !(functionIn.PinValues[0].Bool && functionIn.PinValues[1].Bool);
	return returnVal;
}

FunctionReturn NOR_Boolean(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Bool = !(functionIn.PinValues[0].Bool || functionIn.PinValues[1].Bool);
	return returnVal;
}

FunctionReturn NOT_Boolean(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Bool = !(functionIn.PinValues[0].Bool);
	return returnVal;
}

FunctionReturn Not_Equal_Boolean(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Bool = !(functionIn.PinValues[0].Bool == functionIn.PinValues[1].Bool);
	return returnVal;
}

FunctionReturn OR_Boolean(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Bool = (functionIn.PinValues[0].Bool || functionIn.PinValues[1].Bool);
	return returnVal;
}

FunctionReturn XOR_Boolean(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Bool = ((functionIn.PinValues[0].Bool || functionIn.PinValues[1].Bool) && !(functionIn.PinValues[0].Bool && functionIn.PinValues[1].Bool));
	return returnVal;
}

//Float Operators

FunctionReturn Modulo_Float(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Float = std::fmod(functionIn.PinValues[0].Float, functionIn.PinValues[1].Float);
	return returnVal;
}

FunctionReturn Absolute_Float(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Float = Dymatic::Math::Absolute(functionIn.PinValues[0].Float);
	return returnVal;
}

FunctionReturn Ceil(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Int = std::ceil(functionIn.PinValues[0].Float);
	return returnVal;
}

FunctionReturn Clamp_Float(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Float = std::clamp(functionIn.PinValues[0].Float, functionIn.PinValues[1].Float, functionIn.PinValues[2].Float);
	return returnVal;
}

//Make Literal Nodes

FunctionReturn Make_Bool(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Bool = functionIn.PinValues[0].Bool;
	return returnVal;
}

FunctionReturn Make_Int(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Int = functionIn.PinValues[0].Int;
	return returnVal;
}

FunctionReturn Make_Float(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].Float = functionIn.PinValues[0].Float;
	return returnVal;
}

FunctionReturn Make_String(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues[0].String = functionIn.PinValues[0].String;
	return returnVal;
}

//---------------------------------------------------------------//

FunctionReturn Branch(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.Executable = functionIn.PinValues[1].Bool ? 0 : 1;
	return returnVal;
}

FunctionReturn Random_Bool(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues.push_back({});

	returnVal.PinValues[1].Bool = Dymatic::Math::GetRandomInRange(0, 2) == 1;
	return returnVal;
}

FunctionReturn Random_Float(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});
	returnVal.PinValues.push_back({});

	returnVal.PinValues[1].Float = Dymatic::Math::GetRandomInRange(0, 9999999);
	return returnVal;
}

//Conversions

FunctionReturn Int_To_Bool(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].Bool = functionIn.PinValues[0].Int;

	return returnVal;
}

FunctionReturn Float_To_Bool(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].Bool = functionIn.PinValues[0].Float;

	return returnVal;
}

FunctionReturn String_To_Bool(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].Bool = (functionIn.PinValues[0].String == "true" || functionIn.PinValues[0].String == "True") ? true : false;
	return returnVal;
}

FunctionReturn Bool_To_Int(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].Int = functionIn.PinValues[0].Bool ? 1 : 0;

	return returnVal;
}

FunctionReturn Float_To_Int(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].Int = functionIn.PinValues[0].Float;

	return returnVal;
}

FunctionReturn String_To_Int(FunctionReturn functionIn)
{
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
	return returnVal;
}

FunctionReturn Bool_To_Float(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].Float = functionIn.PinValues[0].Bool ? 1.0f : 0.0f;

	return returnVal;
}

FunctionReturn Int_To_Float(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].Float = functionIn.PinValues[0].Int;

	return returnVal;
}

FunctionReturn String_To_Float(FunctionReturn functionIn)
{
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
	return returnVal;
}

FunctionReturn Bool_To_String(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].String = functionIn.PinValues[0].Bool ? "true" : "false";

	return returnVal;
}

FunctionReturn Int_To_String(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].String = std::to_string(functionIn.PinValues[0].Int);

	return returnVal;
}

FunctionReturn Float_To_String(FunctionReturn functionIn)
{
	FunctionReturn returnVal;
	returnVal.PinValues.push_back({});

	returnVal.PinValues[0].String = std::to_string(functionIn.PinValues[0].Float);

	return returnVal;
}