#include "dypch.h"
#include "Dymatic/Math/Fraction.h"

#include <numeric>

namespace Dymatic {


	Fraction::Fraction(int numerator, int denominator)
		: Numerator(numerator), Denominator(denominator)
	{
		DY_CORE_VERIFY(Denominator != 0);
		Simplify();
	}

	void Fraction::Simplify()
	{
		int gcd = std::gcd(Numerator, Denominator);
		Numerator /= gcd;
		Denominator /= gcd;

		// Keep denominator positive
		if (Denominator < 0)
		{
			Numerator = -Numerator;
			Denominator = -Denominator;
		}
	}

	Fraction Fraction::operator+(const Fraction& other) const
	{
		int numerator = Numerator * other.Denominator + other.Numerator * Denominator;
		int denominator = Denominator * other.Denominator;
		return Fraction(numerator, denominator);
	}

	Fraction Fraction::operator-(const Fraction& other) const
	{
		int numerator = Numerator * other.Denominator - other.Numerator * Denominator;
		int denominator = Denominator * other.Denominator;
		return Fraction(numerator, denominator);
	}

	Fraction Fraction::operator*(const Fraction& other) const
	{
		return Fraction(Numerator * other.Numerator, Denominator * other.Denominator);
	}

	Fraction Fraction::operator/(const Fraction& other) const
	{
		return Fraction(Numerator * other.Denominator, Denominator * other.Numerator);
	}

	float Fraction::GetFloat() const
	{
		return (float)Numerator / (float)Denominator;
	}

}