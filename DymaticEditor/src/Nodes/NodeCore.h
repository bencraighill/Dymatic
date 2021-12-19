#pragma once
#include "Dymatic/Core/Base.h"

enum Meta
{
	Keywords,
	Category,
	Pure,
	DisplayName,
	CompactNodeTitle,
	NoPinLabels
};

// Only used by Node Interpreter when importing functions from C++ files.
#define DYCLASS(...)
#define DYFUNCTION(...)
#define DYENUM(...)

#define check(condition) DY_ASSERT(condition);

class DYBlueprintFunctionLibrary
{
};

class StateStack
{
public:
	StateStack() = default;

	inline void Push(int32_t value) { m_Stack.push_back(value); }
	inline int32_t Pop() { auto back = m_Stack.back(); m_Stack.pop_back(); return back; }
	inline int32_t Num() { return m_Stack.size(); }
private:
	std::vector<int32_t> m_Stack;
};

// Data Type Defines
typedef bool Bool;
typedef int Int;
typedef float Float;
typedef std::string String;
typedef glm::vec3 Vector;
typedef glm::vec4 Vector4;
typedef glm::vec4 Color;