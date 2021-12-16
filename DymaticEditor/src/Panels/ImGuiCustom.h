#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <array>

// Calc Library
#include <string>
#include <stack>
#include <utility>
#include <sstream>
#include <list>
#include <cassert>
#include <map>
#include <array>
#include <cfloat>
#include <cmath>

namespace ImGui::Custom {

	enum ImGuiCol_
	{
		ImGuiCol_CheckboxBg,
		ImGuiCol_CheckboxBgHovered,
		ImGuiCol_CheckboxBgActive,
		ImGuiCol_CheckboxBgTicked,
		ImGuiCol_CheckboxBgHoveredTicked,
		ImGuiCol_FileBackground,
		ImGuiCol_FileIcon,
		ImGuiCol_FileSelected,
		ImGuiCol_FileHovered,
		IMGUI_CUSTOM_COLOR_SIZE
	};

	ImVec4& GetImGuiCustomColorValue(ImGuiCol_ col);

	bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f);
	bool ButtonCornersEx(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags, float rounding, ImDrawCornerFlags cornerFlags, bool active = false, ImVec2 frameOffsetMin = ImVec2{ 0, 0 }, ImVec2 frameOffsetMax = ImVec2{ 0, 0 });
	bool Checkbox(const char* label, bool* v);

	// Text Box Calculator
	template<typename NumType>
	class Calculator
	{
	public:
		typedef NumType NumberType;
		//typedef float NumberType;
		typedef NumberType(*OperatorFunction)(NumberType op0, NumberType op2);
		enum AssociationType
		{
			ASSOC_NONE = 0,
			ASSOC_LEFT,
			ASSOC_RIGHT
		};

		static const short UnaryMinusPrecedence = 10;

		struct  Operator
		{
			char sym;
			short prec;
			AssociationType assoc;
			bool unary;
			OperatorFunction eval;
		};
	private:
		static NumberType op_uminus(NumberType op0, NumberType op1) { return -op0; }
		std::stack<Operator*> _opStack;
		std::stack<NumberType> _valStack;
		std::map<char, Operator> _operators;
		Operator startOp;

		struct CalculationItem
		{
			enum ItemType { VALUE, FUNCTION };

			CalculationItem(Operator* op) : _type(FUNCTION), _op(op), _val(FLT_MAX) {}
			CalculationItem(NumberType val) : _type(VALUE), _op(nullptr), _val(val) {}
			inline NumberType GetValue() const { assert(!_op && _type == VALUE); return _val; };
			inline Operator* GetFuncDesc() const { assert(_op && _type == FUNCTION); return _op; };
			inline ItemType GetType() const { return _type; }
		protected:
			ItemType _type;
			Operator* _op;
			NumberType _val;
		};

	public:
		template<size_t SIZE>
		Calculator(const std::array<Operator, SIZE>& op)
		{
			for (size_t i = 0; i < op.size(); ++i)
				_operators.emplace(std::pair<char, Operator>(op[i].sym, op[i]));

			_operators.emplace(std::pair<char, Operator>('(', { '(', 0, Calculator::ASSOC_NONE, 0, nullptr }));
			_operators.emplace(std::pair<char, Operator>(')', { ')', 0, Calculator::ASSOC_NONE, 0, nullptr }));
			_operators.emplace(std::pair<char, Operator>('_', { '_', UnaryMinusPrecedence, Calculator::ASSOC_RIGHT, 1, op_uminus }));

			startOp = { '?', 0, ASSOC_NONE, 0, nullptr };
		}
		~Calculator() {}

		bool IsReserved(char oper) { return oper != '_' && _operators.find(oper) != _operators.end(); }
		NumberType SolveEquation(const std::string& eq)
		{
			if (eq.empty())
				return 0;
			std::list<CalculationItem> rpn = compile(eq);

			for (auto& item : rpn)
			{
				switch (item.GetType())
				{
				case CalculationItem::VALUE:
					_valStack.push(item.GetValue());
					break;
				case CalculationItem::FUNCTION:
					if (!item.GetFuncDesc()->eval)
						throw std::runtime_error(std::string("Missing function pointer for \"") + item.GetFuncDesc()->sym + "\" operator.");

					if (_valStack.empty())
						throw std::runtime_error(std::string("Missing value for operator \"") + item.GetFuncDesc()->sym + "\".");
					NumberType op1 = _valStack.top();  _valStack.pop();
					if (item.GetFuncDesc()->unary)
						_valStack.push(item.GetFuncDesc()->eval(op1, 0));
					else
					{
						if (_valStack.empty())
							throw std::runtime_error(std::string("Missing value for operator \"") + item.GetFuncDesc()->sym + "\".");
						NumberType op0 = _valStack.top();  _valStack.pop();
						_valStack.push(item.GetFuncDesc()->eval(op0, op1));
					}
					break;
				}
			}

			if (_valStack.size() != 1)
				throw std::runtime_error(std::string("Equation was probably unfinished, please check it: \"") + eq + "\".");

			float ret = _valStack.top();
			_valStack.pop();
			return ret;
		}
	protected:
		//! Compile to "Reverse polish notation"
		std::list<CalculationItem> compile(const std::string& infix)
		{
			while (!_opStack.empty())
				_opStack.pop();

			std::list<CalculationItem> output;
			std::istringstream ss(infix);
			char oper;
			Operator* lastOp = &startOp;
			do
			{
				if (isdigit(ss.peek()))
				{
					NumberType value;
					ss >> value;
					output.emplace_back(std::move(value));
					lastOp = nullptr;
				}
				else
				{
					oper = ss.get();
					if (!IsReserved(oper))
						throw std::runtime_error(std::string("Encountered unknown operand \"") + oper + "\" in equation");

					Operator* curOp = &_operators[oper];
					if (curOp->sym == '-' && lastOp && (lastOp == &startOp || lastOp->sym != ')'))
						curOp = &_operators['_'];

					if (oper == '(')
						_opStack.push(curOp);
					else if (oper == ')')
					{
						while (!_opStack.empty() && _opStack.top()->sym != '(')
						{
							output.emplace_back(_opStack.top());
							_opStack.pop();
							if (_opStack.empty())
								throw std::runtime_error("There is missing matching bracket in equation.");
						}
						_opStack.pop();
					}
					else
					{
						while (
							!_opStack.empty() &&
							((curOp->assoc == ASSOC_LEFT && curOp->prec <= _opStack.top()->prec) ||
								(curOp->assoc == ASSOC_RIGHT && curOp->prec < _opStack.top()->prec))
							)
						{
							output.emplace_back(_opStack.top());
							_opStack.pop();
						}
						_opStack.push(curOp);
					}
					lastOp = curOp;
				}
				Calculator::SkipOverSpace(ss);
			} while (ss.good() && ss.peek() != EOF);

			while (!_opStack.empty())
			{
				output.emplace_back(_opStack.top());
				_opStack.pop();
			}

			return output;
		}

		static void SkipOverSpace(std::istream& stream) { while (stream.good() && isspace(stream.peek())) stream.get(); }
	};



	static double op_plus(double op0, double op1) { return op0 + op1; }
	static double op_minus(double op0, double op1) { return op0 - op1; }
	static double op_multip(double op0, double op1) { return op0 * op1; }
	static double op_divide(double op0, double op1) { return op0 / op1; }
	static double op_mod(double op0, double op1) { return fmod(op0, op1); }
	static double op_power(double op0, double op1) { return pow(op0, op1); }
	static double op_and(double op0, double op1) { return double((int)op0 & (int)op1); }
	static double op_or(double op0, double op1) { return double((int)op0 | (int)op1); }

	static const std::array<Calculator<double>::Operator, 8> calc_gramar = { {
		{ '&', 5, Calculator<double>::ASSOC_LEFT, 0, op_and },
		{ '|', 5, Calculator<double>::ASSOC_LEFT, 0, op_or },
		{ '^', 4, Calculator<double>::ASSOC_RIGHT, 0, op_power },
		{ '*', 3, Calculator<double>::ASSOC_LEFT, 0, op_multip },
		{ '/', 3, Calculator<double>::ASSOC_LEFT, 0, op_divide },
		{ '%', 3, Calculator<double>::ASSOC_LEFT, 0, op_mod },
		{ '+', 2, Calculator<double>::ASSOC_LEFT, 0, op_plus },
		{ '-', 2, Calculator<double>::ASSOC_LEFT, 0, op_minus },
	} };
	static Calculator<double> GCalculator(calc_gramar);

	template<class __type>
	bool InputScalarCalc(const char* label, __type* val, const char* fmt, ImGuiInputTextFlags flags)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		char buf[64];
		sprintf(buf, fmt, *val);
		ImGui::PushID(label);

		bool resa = false;
		if (ImGui::InputText("", buf, IM_ARRAYSIZE(buf), flags | ImGuiInputTextFlags_AutoSelectAll))
		{
			try
			{
				double res = GCalculator.SolveEquation(buf);
				*val = res;
				resa = true;
			}
			catch (std::runtime_error& e)
			{
				// Ignore invalid inputs
			}
		}
		ImGui::PopID();
		return resa;
	}
	
}