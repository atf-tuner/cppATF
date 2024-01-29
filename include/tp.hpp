
#ifndef tp_h
#define tp_h

#include <vector>
#include <functional>
#include <math.h>
#include <memory>

#include "range.hpp"

namespace atf
{


template< typename T, typename range_t, typename callable >
class tp_t
{
  friend class exploration_engine;

  public:
    using type = T;
  
        tp_t( const std::string& name, range_t range, const callable& predicate = [](T){ return true; } )
      : _name( name ), _range( range ), _predicate( predicate ), _act_elem( std::make_shared<T>() )
    {}
  
  
    std::string name() const
    {
      return _name;
    }
  
        bool get_next_value( T& elem )
    {
      // get next element
      if( !_range.next_elem( elem ) )
        return false;

      // while predicate is not fullfilled on "elem" then get next element
      while( !_predicate( elem ) )
        if( !_range.next_elem( elem ) )
          return false;

      *_act_elem = elem;
      return true;
    }
  
    operator T() const
    {
      return *_act_elem;
    }
  
  
    auto cast() const
    {
      return *_act_elem;
    }
  
  
    range const* get_range_ptr() const
    {
      return &_range;
    }
  
  

  private:
    const std::string        _name;
          range_t            _range;
    const callable           _predicate;
          std::shared_ptr<T> _act_elem;
};


template< typename range_t, typename callable, typename T = typename range_t::out_type >
auto tuning_parameter(const std::string& name, range_t range, const callable& predicate )
{
  return tp_t<T,range_t,callable>( name, range, predicate );
}


// function for deducing class template parameters of "tp_t"
template< typename range_t, typename T = typename range_t::in_type >
auto tuning_parameter(const std::string& name, range_t range )
{
  return tuning_parameter(name, range, [](T) { return true; });
}



// enables initializer lists in tp definition

template< typename T, typename callable >
auto tuning_parameter(const std::string& name, const std::initializer_list<T>& elems, const callable& predicate )
{
  return tp_t<T,set<T>,callable>( name, set< typename std::remove_reference<T>::type >( elems ), predicate );
}



// enables vectors in tp definition
template< typename T >
auto tuning_parameter(const std::string& name, const std::vector<T>& elems )
{
  return tuning_parameter(name, elems, [](T) { return true; });
}

template< typename T, typename callable >
auto tuning_parameter(const std::string& name, const std::vector<T>& elems, const callable& predicate )
{
  return tp_t<T,set<T>,callable>( name, set< typename std::remove_reference<T>::type >( elems ), predicate );
}
 

// function for deducing class template parameters of "tp_t"
template< typename T >
auto tuning_parameter(const std::string& name, const std::initializer_list<T>& elems )
{
  return tuning_parameter(name, elems, [](T) { return true; });
}
tp_t<std::string,set<std::string>,std::function<bool(std::string)>> tuning_parameter(const std::string& name, const std::initializer_list<const char*>& elems )
{
  std::vector<std::string> elems_as_strings(elems.begin(), elems.end());
  return {name, set<std::string>(elems_as_strings), [](std::string) -> bool { return true; } };
}



} // namespace "atf"

#endif /* tp_h */
