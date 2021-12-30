#pragma once

#include "NodeCore.h"

DYCLASS()
class DYTestNodeLibrary : public DYBlueprintFunctionLibrary
{
public:
	DYFUNCTION(BlueprintCallable, Meta = (ExpandEnumAsExecs = "Branches", Keywords = "Branching Thingo"), Category = "Flow Control")
	static  String  SomeFunFunction   (DYPARAM(Ref) float& fReference, const String& text, bool& Success, float displayTime = 1.0f);

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

	DYFUNCTION(Category = "Debug|Testing", Pure, DisplayName = "AND Boolean", CompactNodeTitle = "AND", NoPinLabels)
	static bool BooleanAND(bool a, bool b);

	DYFUNCTION(Category = "Math|Float", Keywords = "+", Pure, DisplayName = "Float Addition", CompactNodeTitle = "+", NoPinLabels)
	static float FloatAddition(float a, float b);

	DYFUNCTION(DisplayName = "ToString (float)", Category = "Math|Conversions", Keywords = "convert cast", Pure, CompactNodeTitle = "->", ConversionAutocast)
	static String Conv_FloatToString(Float InFloat);

	DYFUNCTION(DisplayName = "Make Vector", Category = "Math|Vector", Keywords = "create", Pure, NativeMakeFunc)
	static Vector Make_Vector(float x, float y, float z);

	DYFUNCTION(DisplayName = "Make Float", Category = "Math|Float", Keywords = "create", Pure, NativeMakeFunc)
	static Float Make_Float(bool bit_a, bool bit_b, bool bit_c, bool bit_d);

	DYFUNCTION(DisplayName = "Break Float", Category = "Math|Float", Keywords = "split", Pure, NativeBreakFunc)
	static void Break_Float(float Float, bool& bit_a, bool& bit_b, bool& bit_c, bool& bit_d);

	DYFUNCTION(DisplayName = "Break Color", Category = "Math|Vector", Keywords = "split", Pure, NativeBreakFunc)
	static void Break_Color(Color color, float& r, float& g, float& b, float& a);

	DYFUNCTION(DisplayName = "Make Color", Pure, NativeMakeFunc)
	static Color Make_Color(float r, float g, float b, float a);

	DYFUNCTION(Pure)
	static Byte Literal_Byte(Byte InByte);

	DYFUNCTION(Pure, NoPinLabels)
	static Byte ConstructByte(bool bit_1, bool bit_2, bool bit_3, bool bit_4, bool bit_5, bool bit_6, bool bit_7, bool bit_8);

	DYFUNCTION(DisplayName = "ToString (byte)", Category = "Math|Byte", Keywords = "convert cast", Pure, CompactNodeTitle = "->", ConversionAutocast)
	static String Conv_ByteToString(Byte InByte);
};

String DYTestNodeLibrary::SomeFunFunction(float& fReference, const String& text, bool& success, float displayTime /*= 1.0f*/)
{
	success = true;
	return text + std::to_string(displayTime);
}

void DYTestNodeLibrary::PrintString(String InString, Color TextColor)
{
	DY_CORE_INFO(InString);
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

float DYTestNodeLibrary::FloatAddition(float a, float b)
{
	return a + b;
}

String DYTestNodeLibrary::Conv_FloatToString(Float InFloat)
{
	return std::to_string(InFloat);
}

Vector DYTestNodeLibrary::Make_Vector(float x, float y, float z)
{
	return { x, y, z };
}

Float DYTestNodeLibrary::Make_Float(bool bit_a, bool bit_b, bool bit_c, bool bit_d)
{
	return bit_a + bit_b + bit_c + bit_d;
}

void DYTestNodeLibrary::Break_Float(float Float, bool& bit_a, bool& bit_b, bool& bit_c, bool& bit_d)
{
}

void DYTestNodeLibrary::Break_Color(Color color, float& r, float& g, float& b, float& a)
{
	r = color.r;
	g = color.g;
	b = color.b;
	a = color.a;
}

Color DYTestNodeLibrary::Make_Color(float r, float g, float b, float a)
{
	return { r, g, b, a };
}

Byte DYTestNodeLibrary::Literal_Byte(Byte InByte)
{
	return InByte;
}

Byte DYTestNodeLibrary::ConstructByte(bool bit_1, bool bit_2, bool bit_3, bool bit_4, bool bit_5, bool bit_6, bool bit_7, bool bit_8)
{
	return { (unsigned char)(
	(bit_1)			+
	(bit_2 * 2)		+
	(bit_3 * 4)		+
	(bit_4 * 8)		+
	(bit_5 * 16)	+
	(bit_6 * 32)	+
	(bit_7 * 64)	+
	(bit_8 * 128)
	) };
}

String DYTestNodeLibrary::Conv_ByteToString(Byte InByte)
{
	std::string byteString;
	for (int i = 0; i != 8; i++) {
		byteString = std::to_string((InByte & (1 << i)) != 0) + byteString;
	}
	return byteString;
}
