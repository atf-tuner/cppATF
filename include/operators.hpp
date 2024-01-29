
#ifndef operators_h
#define operators_h

#include "op_wrapper.hpp"


namespace atf {


template<typename>
struct is_tp_t : std::false_type {};

template< typename T, typename range_t, typename callable >
struct is_tp_t< tp_t<T,range_t,callable> > : std::true_type {};

template< typename T, typename range_t, typename callable >
struct is_tp_t< tp_t<T,range_t,callable>& > : std::true_type {};

template< typename T, typename range_t, typename callable >
struct is_tp_t< tp_t<T,range_t,callable>&& > : std::true_type {};


template<typename>
struct is_operator_wrapper_class : std::false_type {};

template< typename T_lhs, typename T_rhs, typename callable >
struct is_operator_wrapper_class< op_wrapper_class<T_lhs,T_rhs,callable> > : std::true_type {};

template< typename T_lhs, typename T_rhs, typename callable >
struct is_operator_wrapper_class< op_wrapper_class<T_lhs,T_rhs,callable>& > : std::true_type {};

template< typename T_lhs, typename T_rhs, typename callable >
struct is_operator_wrapper_class< op_wrapper_class<T_lhs,T_rhs,callable>&& > : std::true_type {};



// addition
auto add_lambda = [](auto x, auto y){ return x + y; };
template< typename T_lhs, typename T_rhs, std::enable_if_t<( is_tp_t<T_lhs>::value || is_tp_t<T_rhs>::value || is_operator_wrapper_class<T_lhs>::value || is_operator_wrapper_class<T_rhs>::value )>* = nullptr >
auto operator+( T_lhs&& lhs, T_rhs&& rhs )
{
  return op_wrapper( std::forward<T_lhs>(lhs), std::forward<T_rhs>(rhs), add_lambda );
}



// multiplication
auto mult_lambda = [](auto x, auto y){ return x * y; };
template< typename T_lhs, typename T_rhs, std::enable_if_t<( is_tp_t<T_lhs>::value || is_tp_t<T_rhs>::value || is_operator_wrapper_class<T_lhs>::value || is_operator_wrapper_class<T_rhs>::value )>* = nullptr >
auto operator*( T_lhs&& lhs, T_rhs&& rhs )
{
  return op_wrapper( std::forward<T_lhs>(lhs), std::forward<T_rhs>(rhs), mult_lambda );
}


// subtraction
auto minus_lambda = [](auto x, auto y){ return x - y; };
template< typename T_lhs, typename T_rhs, std::enable_if_t<( is_tp_t<T_lhs>::value || is_tp_t<T_rhs>::value || is_operator_wrapper_class<T_lhs>::value || is_operator_wrapper_class<T_rhs>::value )>* = nullptr >
auto operator-( T_lhs&& lhs, T_rhs&& rhs )
{
  return op_wrapper( std::forward<T_lhs>(lhs), std::forward<T_rhs>(rhs), minus_lambda );
}


// division
auto div_lambda = [](auto x, auto y){ return x / y; };
template< typename T_lhs, typename T_rhs, std::enable_if_t<( is_tp_t<T_lhs>::value || is_tp_t<T_rhs>::value || is_operator_wrapper_class<T_lhs>::value || is_operator_wrapper_class<T_rhs>::value )>* = nullptr >
auto operator/( T_lhs&& lhs, T_rhs&& rhs )
{
  return op_wrapper( std::forward<T_lhs>(lhs), std::forward<T_rhs>(rhs), div_lambda );
}



// modulo
auto mod_lambda = [](auto x, auto y){ return x % y; };
template< typename T_lhs, typename T_rhs, std::enable_if_t<( is_tp_t<T_lhs>::value || is_tp_t<T_rhs>::value || is_operator_wrapper_class<T_lhs>::value || is_operator_wrapper_class<T_rhs>::value )>* = nullptr >
auto operator%( T_lhs&& lhs, T_rhs&& rhs )
{
  return op_wrapper( std::forward<T_lhs>(lhs), std::forward<T_rhs>(rhs), mod_lambda );
}



// equal
auto equ_lambda = [](auto x, auto y)->bool{ return x == y; };
template< typename T_lhs, typename T_rhs, std::enable_if_t<( is_tp_t<T_lhs>::value || is_tp_t<T_rhs>::value || is_operator_wrapper_class<T_lhs>::value || is_operator_wrapper_class<T_rhs>::value )>* = nullptr >
auto operator==( T_lhs&& lhs, T_rhs&& rhs )
{
  return op_wrapper( std::forward<T_lhs>(lhs), std::forward<T_rhs>(rhs), equ_lambda );
}



// less
auto less_lambda = [](auto x, auto y)->bool{ return x < y; };
template< typename T_lhs, typename T_rhs, std::enable_if_t<( is_tp_t<T_lhs>::value || is_tp_t<T_rhs>::value || is_operator_wrapper_class<T_lhs>::value || is_operator_wrapper_class<T_rhs>::value )>* = nullptr >
auto operator<( T_lhs&& lhs, T_rhs&& rhs )
{
  return op_wrapper( std::forward<T_lhs>(lhs), std::forward<T_rhs>(rhs), less_lambda );
}



// greater
auto greater_lambda = [](auto x, auto y)->bool{ return x > y; };
template< typename T_lhs, typename T_rhs, std::enable_if_t<( is_tp_t<T_lhs>::value || is_tp_t<T_rhs>::value || is_operator_wrapper_class<T_lhs>::value || is_operator_wrapper_class<T_rhs>::value )>* = nullptr >
auto operator>( T_lhs&& lhs, T_rhs&& rhs )
{
  return op_wrapper( std::forward<T_lhs>(lhs), std::forward<T_rhs>(rhs), greater_lambda );
}



// and
auto and_lambda = [](auto x, auto y)->bool{ return x && y; };
template< typename T_lhs, typename T_rhs, std::enable_if_t<( is_tp_t<T_lhs>::value || is_tp_t<T_rhs>::value || is_operator_wrapper_class<T_lhs>::value || is_operator_wrapper_class<T_rhs>::value )>* = nullptr >
auto operator&&( T_lhs&& lhs, T_rhs&& rhs )
{
  return op_wrapper( std::forward<T_lhs>(lhs), std::forward<T_rhs>(rhs), and_lambda );
}


} // namespace "atf"

#endif /* operators_h */
