
#ifndef predicates_h
#define predicates_h

#include "detail/utility.hpp"
#include "op_wrapper.hpp"

namespace atf {

auto pow_2 = []( auto i )->int{ return pow(2,i); };


// divides
template< typename T> auto divides( const T& M ) {

  return [=]( auto i )->bool{ return (M / i) * i == M; };
}

// multiple_of
template< typename T> auto multiple_of( const T& M )
{
  return [=]( auto i )->bool{ return (i / M) * M == i; };
}

// less than
template< typename T>
auto less_than( const T& M )
{
  return [=]( auto i )->bool{ return i < M; };
}

// greater than
template< typename T>
auto greater_than( const T& M )
{
  return [=]( auto i )->bool{ return i > M; };
}

// less than or equal
template< typename T>
auto less_than_or_eq( const T& M )
{
  return [=]( auto i )->bool{ return i <= M; };
}

// greater than or equal
template< typename T>
auto greater_than_or_eq( const T& M )
{
  return [=]( auto i )->bool{ return i >= M; };
}


// equal
template< typename T>
auto equal( const T& M )
{
  return [=]( auto i )->bool{ return i == M; };
}


// unequal
template< typename T>
auto unequal( const T& M )
{
  return [=]( auto i )->bool{ return i != M; };
}


} // namespace "atf"


// operators
template<	typename func_t_1,
    typename func_t_2,
    typename = ::std::enable_if_t<
        (atf::is_callable_v<::std::decay_t<func_t_1>> ||
         atf::is_generic_callable_v<::std::decay_t<func_t_1>>) &&
        (atf::is_callable_v<::std::decay_t<func_t_2>> ||
         atf::is_generic_callable_v<::std::decay_t<func_t_2>>)
    >
>
auto operator&&( func_t_1 lhs, func_t_2 rhs )
{
  return [=]( auto x )
  {
    if( lhs(x) ==  false )  // enables short circuit evaluation
      return false;
    else
      return static_cast<bool>( rhs(x) );
  };
}


template<	typename func_t_1,
    typename func_t_2,
    typename = ::std::enable_if_t<
        (atf::is_callable_v<::std::decay_t<func_t_1>> ||
         atf::is_generic_callable_v<::std::decay_t<func_t_1>>) &&
        (atf::is_callable_v<::std::decay_t<func_t_2>> ||
         atf::is_generic_callable_v<::std::decay_t<func_t_2>>)
    >
>
auto operator||( func_t_1 lhs, func_t_2 rhs )
{
  return [=]( auto x )
  {
    if( lhs(x) ==  true )  // enables short circuit evaluation
      return true;
    else
      return static_cast<bool>( rhs(x) );
  };
}



#endif /* predicates_h */
