#ifndef utility_h
#define utility_h

#include <type_traits>

namespace atf
{
	namespace detail
	{
		// void_t: Helper alias that sinks all supplied template parameters
		// and is always void.
		template< typename... Ts >
		using void_t = void;

		// is_callable_impl: Implements "is_callable" type-trait using SFINAE.
		// We check for occurence of any valid operator() overload in the given type.

		// Fallback case: this is selected when no operator() is found.
		template< typename Void, typename T >
		struct is_callable_impl
			: ::std::false_type
		{
		};

		// TRUE case
		template< typename T >
		struct is_callable_impl	<void_t<decltype(&T::operator())>, T>
			: ::std::true_type
		{
		};
		//

		template< typename Void, typename T >
		struct is_generic_callable_impl
			: ::std::false_type
		{
		};

		template< typename T >
		struct is_generic_callable_impl	<void_t<decltype(&T::template operator()<bool>)>, T>
			: ::std::true_type
		{
		};
	}

	// is_callable: Type trait that applies to all callable types, excluding function pointers.
	template< typename T >
	using is_callable = detail::is_callable_impl<void, T>;

	template< typename T >
	constexpr bool is_callable_v = is_callable<T>::value;

	template< typename T >
	using is_generic_callable = detail::is_generic_callable_impl<void, T>;

	template< typename T >
	constexpr bool is_generic_callable_v = is_generic_callable<T>::value;
}
#endif
