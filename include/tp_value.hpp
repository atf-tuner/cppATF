
#ifndef tp_value_h
#define tp_value_h

#include <map>

#include "tp_value_node.hpp"
#include "value_type.hpp"

namespace atf
{

class tp_value
{
  public:
    tp_value() = default;
    tp_value(int v) : _value(v) { update_tp(); }
    tp_value(size_t v) : _value(v) { update_tp(); }
    tp_value(float v) : _value(v) { update_tp(); }
    tp_value(double v) : _value(v) { update_tp(); }

    tp_value& operator=( const tp_value& other ) = default;
    
    tp_value( const value_type& value, void* tp_value_ptr )
      : _value( value ), _tp_value_ptr( tp_value_ptr )
    {}
  
    // read / write
    value_type&  value()
    {
      return _value;
    }


    // read only
    const value_type&  value() const
    {
      return _value;
    }
  
  
    void update_tp() const
    {
      switch( _value.type_id() )
      {
        case value_type::root_t:
          assert( false && "should never be reached" );
          throw std::exception();
          break;

        case value_type::int_t:
          *static_cast<int*>( _tp_value_ptr ) = _value.int_val();
          break;

        case value_type::size_t_t:
          *static_cast<size_t*>( _tp_value_ptr ) = _value.size_t_val();
          break;

        case value_type::float_t:
          *static_cast<float*>( _tp_value_ptr ) = _value.float_val();
          break;

        case value_type::double_t:
          *static_cast<double*>( _tp_value_ptr ) = _value.double_val();
          break;

        case value_type::string_t:
          *static_cast<std::string*>( _tp_value_ptr ) = _value.string_val();
          break;

        default:
          throw std::exception();
      }
    }
  
    template< typename T >
    operator T()
    {
      return static_cast<T>( _value );
    }

    operator std::string()
    {
      return static_cast<std::string>( _value );
    }



  
  private:
    value_type _value;
    void*      _tp_value_ptr;
};

// operators
std::ostream& operator<< (std::ostream &out, const tp_value& tp_value )
{
  auto value = tp_value.value();
  return operator<<(out, value);
}
bool operator<( const tp_value& lhs, const tp_value& rhs )
{
//  assert( lhs.name() == rhs.name() );
  return lhs.value() < rhs.value();
}

// typedefs
typedef std::map<std::string, tp_value> configuration;

} // namespace "atf"


#endif /* tp_value_h */
