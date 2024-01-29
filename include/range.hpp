
#ifndef range_h
#define range_h

#include <vector>
#include <assert.h>
#include <limits>

#include <iostream>

#include "value_type.hpp"

namespace atf
{

class range
{
  public:

    virtual size_t size() const = 0;
  
    virtual const value_type operator[]( size_t i ) const = 0;
};



template< typename T_in, typename callable, typename T_out = typename std::result_of<callable(T_in)>::type >
class interval_class : public range
{
  public:
        using in_type  = T_in;
    using out_type = T_out;   
  
    interval_class( T_in begin, T_in end, T_in step_size = static_cast<T_in>( 1 ), callable generator = []( T_in i ){ return i; } )
      : _begin( begin ), _end( end ), _step_size( step_size ), _pos( begin ), _generator( generator )
    {
      assert( _begin >= std::numeric_limits<T_in>::min() );
      assert( _end   <  std::numeric_limits<T_in>::max() ); // not "<=" since, for size_t, "std::numeric_limits<size_t>::max()+1 == 0" and thus "_pos<=_end == true"
    }

    bool next_elem( T_out& elem )
    {
      if( _pos <= _end )
      {
        elem = _generator( _pos );
        _pos += _step_size;
        return true;
      }
      else
      {
        _pos = _begin;
        return false;
      }
    }
  
    size_t size() const
    {
      size_t size = ( (_end - _begin) + 1) /  _step_size;
      
      assert( size > 0 );
      
      return size;
    }
  
    const value_type operator[]( size_t i ) const
    {
      auto elem = _begin + ( _step_size * static_cast<T_in>( i ) );
      return _generator( elem );
    }
  
  private:
    T_in     _begin;
    T_in     _end;
    T_in     _step_size;
    T_in     _pos;
    callable _generator;
  


};


// intervall with: begin, end, step_size, generator
template< typename T_in, typename callable >
auto interval( T_in begin, T_in end, T_in step_size, callable generator )
{
  return interval_class<T_in,callable>( begin, end, step_size, generator );
}

// intervall with: begin, end, step_size
template< typename T_in >
auto interval( T_in begin = std::numeric_limits<T_in>::min(), T_in end = std::numeric_limits<T_in>::max() - static_cast<T_in>( 1 ), T_in step_size = static_cast<T_in>( 1 ) )
{
  return interval<T_in>( begin, end, step_size, []( T_in i ){ return i; } );
}

// intervall with: begin, end, generator
template< typename T_in, typename callable >
auto interval( T_in begin, T_in end, callable generator )
{
  return interval_class<T_in,callable>( begin, end, static_cast<T_in>( 1 ) , generator );
}




template< typename T >
class set : public range
{
  public:
    using in_type  = T;
    using out_type = T;
  
    set( const std::vector<T>& elems )
      : _elems( elems ), _num_elems ( elems.size() ), _value_pos( 0 )
    {}
  
    bool next_elem( T& elem )
    {
      if( _value_pos < _num_elems )
      {
        elem = _elems[ _value_pos++ ];
        return true;
      }
      else
      {
        _value_pos = 0;
        return false;
      }
    }
  
    size_t size() const
    {
      return _num_elems;
    }
  
    const value_type operator[]( size_t i ) const
    {
      return _elems[ i ];
    }
  
  private:
    std::vector<T> _elems;
    size_t         _num_elems;
    size_t         _value_pos;
};

} // namespace "atf"

#endif /* range_h */
