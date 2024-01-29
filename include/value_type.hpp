
#ifndef value_type_h
#define value_type_h

#include <string>

namespace atf
{

class value_type
{
  public:
    enum type_id_t { root_t, bool_t, int_t, size_t_t, float_t, double_t, string_t };
  
    type_id_t type_id() const
    {
      return _type_id;
    }
  
    // ctors
    value_type()
      : _type_id( root_t )
    {}
  
    value_type( const bool&        b   )
      : _type_id( bool_t ), _bool_val( b )
    {}
    value_type( const int&         i   )
      : _type_id( int_t ), _int_val( i )
    {}
    value_type( const size_t&      s_t )
      : _type_id( size_t_t ), _size_t_val( s_t )
    {}
    value_type( const float&       f   )
      : _type_id( float_t ), _float_val( f )
    {}
    value_type( const double&      d   )
      : _type_id( double_t ), _double_val( d )
    {}
    value_type( const std::string& s   )
      : _type_id( string_t ), _string_val( s )
    {}
  

  
    // access values
    bool bool_val() const{
      switch( this->type_id() )
      {
        case root_t:
          assert( false && "should never be reached" );
          throw std::exception();
          break;

        case bool_t:
          return _bool_val;
          break;

        case int_t:
          return _int_val;
          break;

        case size_t_t:
          return _size_t_val;
          break;

        case float_t:
          return _float_val;
          break;

        case double_t:
          return _double_val;
          break;

        case string_t:
          assert(false && "no cast from std::string to bool" );
          throw std::exception();
          break;

        default:
          throw std::exception();
      }
    }
    int int_val() const {
      switch( this->type_id() )
      {
        case root_t:
          assert( false && "should never be reached" );
          throw std::exception();
          break;

        case bool_t:
          return _bool_val;
          break;

        case int_t:
          return _int_val;
          break;

        case size_t_t:
          if( _size_t_val <= std::numeric_limits<int>::max() )
            return (int)_size_t_val;
          else
          {
            assert(false && "implicit conversion from size_t to int loses integer precision" );
            throw std::exception();
          }
          break;

        case float_t:
          return _float_val;
          break;

        case double_t:
          return _double_val;
          break;

        case string_t:
          assert(false && "no cast from std::string to int" );
          throw std::exception();
          break;

        default:
          throw std::exception();
      }
    }
    size_t size_t_val() const
    {
      switch( this->type_id() )
      {
        case root_t:
          assert( false && "should never be reached" );
          throw std::exception();
          break;

        case bool_t:
          return _bool_val;
          break;

        case int_t:
          return _int_val;
          break;

        case size_t_t:
          return _size_t_val;
          break;

        case float_t:
          return _float_val;
          break;

        case double_t:
          return _double_val;
          break;

        case string_t:
          assert(false && "no cast from std::string to size_t" );
          throw std::exception();
          break;

        default:
          throw std::exception();
      }
    }
    float float_val() const
    {
      switch( this->type_id() )
      {
        case root_t:
          assert( false && "should never be reached" );
          throw std::exception();
          break;

        case bool_t:
          return _bool_val;
          break;

        case int_t:
          return _int_val;
          break;

        case size_t_t:
          return _size_t_val;
          break;

        case float_t:
          return _float_val;
          break;

        case double_t:
          return _double_val;
          break;

        case string_t:
          assert(false && "no cast from std::string to float" );
          throw std::exception();
          break;

        default:
          throw std::exception();
      }
    }
    double double_val() const
    {
      switch( this->type_id() )
      {
        case root_t:
          assert( false && "should never be reached" );
          throw std::exception();
          break;

        case bool_t:
          return _bool_val;
          break;

        case int_t:
          return _int_val;
          break;

        case size_t_t:
          return (int)_size_t_val;
          break;

        case float_t:
          return _float_val;
          break;

        case double_t:
          return _double_val;
          break;

        case string_t:
          assert(false && "no cast from std::string to double" );
          throw std::exception();
          break;

        default:
          throw std::exception();
      }
    }
    std::string string_val() const
    {
      switch( this->type_id() )
      {
        case root_t:
          assert( false && "should never be reached" );
          throw std::exception();
          break;

        case bool_t:
          if(_bool_val == true)
            return "true";
          else
            return "false";
          break;

        case int_t:
          return std::to_string( _int_val );
          break;

        case size_t_t:
          return std::to_string( _size_t_val );
          break;

        case float_t:
          return std::to_string( _float_val );
          break;

        case double_t:
          return std::to_string( _double_val );
          break;

        case string_t:
          return _string_val;
          break;

        default:
          throw std::exception();
      }

      assert( false && "should never be reached" );
      return "";
    }
  


  
  
    // implicit cast operators
    operator bool() const
    {
      return this->bool_val();
    }
    operator int() const
    {
      return this->int_val();
    }
    operator size_t() const
    {
      return this->size_t_val();
    }
    operator float() const
    {
      return this->float_val();
    }
    operator double() const
    {
      return this->double_val();
    }
    operator std::string() const
    {
      return this->string_val();
    }
  

  
    // c-ctor, c-assignment and ddtor have to be explicitly stated due to std::string in union member
    value_type( const value_type& other )
    {
      switch( other.type_id() )
      {
        case root_t:
          _type_id = root_t;
          break;

        case bool_t:
          _type_id  = bool_t;
          _bool_val = other._bool_val;
          break;

        case int_t:
          _type_id = int_t;
          _int_val = other._int_val;
          break;

        case size_t_t:
          _type_id    = size_t_t;
          _size_t_val = other._size_t_val;
          break;

        case float_t:
          _type_id    = float_t;
          _float_val = other._float_val;
          break;

        case double_t:
          _type_id    = double_t;
          _double_val = other._double_val;
          break;

        case string_t:
          _type_id = string_t;
          new (&_string_val) auto( other._string_val );
          break;

        default:
          throw std::exception();
      }

    }
  
    value_type& operator=( const value_type& other )
    {
      // check for self-assignment
      if( &other == this )
          return *this;

      switch( other.type_id() )
      {
        case root_t:
          assert( false );       return *this;
          break;

        case bool_t:
          this->_type_id = bool_t;
          this->_bool_val = other._bool_val;
          return *this;
          break;

        case int_t:
          this->_type_id = int_t;
          this->_int_val = other._int_val;
          return *this;
          break;

        case size_t_t:
          this->_type_id    = size_t_t;
          this->_size_t_val = other._size_t_val;
          return *this;
          break;

        case float_t:
          this->_type_id   = float_t;
          this->_float_val = other._float_val;
          return *this;
          break;

        case double_t:
          this->_type_id    = double_t;
          this->_double_val = other._double_val;
          return *this;
          break;

        case string_t:
          this->_type_id    = string_t;
          this->_string_val = other._string_val;
          return *this;
          break;

        default:
          throw std::exception();
      }

    }
  
    ~value_type()
    {
      switch( this->type_id() )
      {
        case root_t:
        case bool_t:
        case int_t:
        case size_t_t:
        case float_t:
        case double_t:
          break; // trivially destructible, no need to do anything

        case string_t:
          _string_val.~basic_string();       break;

        default:
//          throw std::runtime_error("unknown type");
          std::terminate();
      }
    }
  
  private:
    type_id_t _type_id;
  
    union     {
      bool        _bool_val;
      int         _int_val;
      size_t      _size_t_val;
      float       _float_val;
      double      _double_val;
      std::string _string_val;
    };
};

// overloaded operators
std::ostream& operator<< (std::ostream &out, const value_type& value )
{
  switch( value.type_id() )
  {
    case value_type::bool_t:
      out << static_cast<bool>( value );
      break;

    case value_type::int_t:
      out << static_cast<int>( value );
      break;

    case value_type::size_t_t:
      out << static_cast<size_t>( value );
      break;

    case value_type::float_t:
      out << static_cast<float>( value );
      break;

    case value_type::double_t:
      out << static_cast<double>( value );
      break;

    case value_type::string_t:
      out << static_cast<std::string>( value );
      break;

    default:
      throw std::exception();
  }

  return out;
}


bool operator!=( const value_type& lhs, const value_type& rhs )
{
  assert( lhs.type_id() == rhs.type_id() );
  auto type_id = lhs.type_id();

  switch( type_id )
  {
    case value_type::bool_t:
      return ( lhs.bool_val() != rhs.bool_val() );
      break;

    case value_type::int_t:
      return ( lhs.int_val() != rhs.int_val() );
      break;

    case value_type::size_t_t:
      return ( lhs.size_t_val() != rhs.size_t_val() );
      break;

    case value_type::float_t:
      return ( lhs.float_val() != rhs.float_val() );
      break;

    case value_type::double_t:
      return ( lhs.double_val() != rhs.double_val() );
      break;

    case value_type::string_t:
      return ( lhs.string_val() != rhs.string_val() );
      break;

    default:
      throw std::exception();
  }
}


bool operator<( const value_type& lhs, const value_type& rhs )
{
  assert( lhs.type_id() == rhs.type_id() );
  auto type_id = lhs.type_id();

  switch( type_id )
  {
    case value_type::bool_t:
      return ( lhs.bool_val() < rhs.bool_val() );
      break;

    case value_type::int_t:
      return ( lhs.int_val() < rhs.int_val() );
      break;

    case value_type::size_t_t:
      return ( lhs.size_t_val() < rhs.size_t_val() );
      break;

    case value_type::float_t:
      return ( lhs.float_val() < rhs.float_val() );
      break;

    case value_type::double_t:
      return ( lhs.double_val() < rhs.double_val() );
      break;

    case value_type::string_t:
      return ( lhs.string_val() < rhs.string_val() );
      break;

    default:
      throw std::exception();
  }
}

} // namespace "atf"

#endif /* value_type_h */
