#pragma once

#include "NodeCore.h"

DYCLASS()
class DYTestNodeLibrary : public DYBlueprintFunctionLibrary
{
public:
	DYFUNCTION(BlueprintCallable, Meta = (ExpandEnumAsExecs = "Branches", Keywords = "Branching Thingo"), Category = "Flow Control")
	static  String  SomeFunFunction   (const String& text, bool& success, float displayTime = 1.0f);

	DYFUNCTION(Category = "Utilities|String", Keywords = "log print")
	/**
	 *Prints a string to the log, and optionally, to the screen
	 *
	 *@param	InString		The String to log out
	 *@param	TextColor		The color of the text to display
	*/
	static void PrintString (String InString, Color TextColor);

	DYFUNCTION(Category = "Debug|Testing")
	static String MakeString(String InString);

	DYFUNCTION(Category = "Debug|Testing", Pure)
	static float Float4Addition(float a, float b, float c, float d, bool Add = true);

	DYFUNCTION(Category = "Debug|Testing", Pure, DisplayName = "AND")
	static bool BooleanAND(bool a, bool b);
};

String DYTestNodeLibrary::SomeFunFunction(const String& text, bool& success, float displayTime /*= 1.0f*/)
{
	return text + std::to_string(displayTime);
}

void DYTestNodeLibrary::PrintString(String InString, Color TextColor)
{
	DY_CORE_INFO("\"{0}\" --> {1}", InString, TextColor);
}

String DYTestNodeLibrary::MakeString(String InString)
{
	return InString;
}

float DYTestNodeLibrary::Float4Addition(float a, float b, float c, float d, bool Add /*= true*/)
{
	if (Add)
		return a + b + c + d;
	else
		return a - b - c - d;
}

bool DYTestNodeLibrary::BooleanAND(bool a, bool b)
{
	return a && b;
}
