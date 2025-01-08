#pragma once

#include <functional>

namespace Dymatic {

	template<typename T>
	struct FunctionTraits;

	// Callable Types (e.g. Lambdas)
	template <typename T>
	struct FunctionTraits : FunctionTraits<decltype(&T::operator())> {};

	// Member Functions
	template <typename R, typename C, typename... Args>
	struct FunctionTraits<R(C::*)(Args...) const>
	{
		using ResultType = R;
		static constexpr size_t ArgumentCount = sizeof...(Args);

		template <size_t i>
		struct Argument
		{
			using Type = typename std::tuple_element<i, std::tuple<Args...>>::type;
		};
	};

	// Free and Static Member Functions
	template <typename R, typename... Args>
	struct FunctionTraits<R(*)(Args...)>
	{
		using ResultType = R;
		static constexpr size_t ArgumentCount = sizeof...(Args);

		template <size_t i>
		struct Argument
		{
			using Type = typename std::tuple_element<i, std::tuple<Args...>>::type;
		};
	};

}