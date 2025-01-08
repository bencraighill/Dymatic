#pragma once

#include "Dymatic/Core/Base.h"

#define DY_USING_FRACTION

namespace Dymatic {

	struct Fraction
	{
		int Numerator;
		int Denominator;

		Fraction(int numerator = 1, int denominator = 1);

		void Simplify();
		
		Fraction operator+(const Fraction& other) const;
		Fraction operator-(const Fraction& other) const;
		Fraction operator*(const Fraction& other) const;
		Fraction operator/(const Fraction& other) const;

		float GetFloat() const;
	};

}