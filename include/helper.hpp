
#ifndef helper_h
#define helper_h

#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <random>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <string>
#include "search_technique.hpp"

namespace atf {

  // helper for large vectors
  template< typename T >
  class sparse_vector
  {
    public:
      sparse_vector() = default;
      sparse_vector( const size_t size, const T& default_value )
        : _size( size ), _default_value( default_value )
      {}
    

      T& operator[]( const size_t& index )
      {

        try
        {
          return _index_value_pairs.at( index );
        }
        catch( std::out_of_range )
        {
          _index_value_pairs[ index ] = _default_value;
          return _index_value_pairs[ index ];
        }
      }
    
    
      size_t size() const
      {
        return _size;
      }
    
    
    private:
      size_t               _size;
      T                    _default_value;
      std::map< size_t, T> _index_value_pairs;
    
  };



template< typename T >
struct eval_t
{
  using type = T;
};


template< typename T >
struct casted_eval_t
{
  using type = decltype( std::declval<T>().cast() );
};


template<>
struct casted_eval_t< std::string >
{
  using type = std::string;
};


template< typename T >
struct T_res_eval_t
{
  using type = typename T::T_res;
};

template< class T >
std::string to_string(const T& t)
{
  std::ostringstream oss; // create a stream
  oss << t;               // insert value to stream
  return oss.str();       // extract value and return
}

namespace data {

template<typename T, typename std::enable_if<std::is_same<T, bool>::value, bool>::type = true>
void fill_with_random_val( T& out, T min, T max )
{
  std::default_random_engine dre{std::random_device()()};
  std::uniform_int_distribution<int> uid{min ? 1 : 0, max ? 1 : 0};
  out = uid(dre) == 1;
}

template<typename T, typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value, bool>::type = true>
void fill_with_random_val( T& out, T min, T max )
{
  std::default_random_engine dre{std::random_device()()};
  std::uniform_int_distribution<T> uid{min, max};
  out = uid(dre);
}

template<typename T, typename std::enable_if<std::is_floating_point<T>::value, bool>::type = true>
void fill_with_random_val( T& out, T min, T max )
{
  std::default_random_engine dre{std::random_device()()};
  std::uniform_real_distribution<T> urd{min, max};
  out = max - urd(dre);
}

template<typename T, typename std::enable_if<std::is_same<T, bool>::value, bool>::type = true>
void fill_with_random_val( std::vector<T>& out, T min, T max )
{
  std::default_random_engine dre{std::random_device()()};
  std::uniform_int_distribution<int> uid{min ? 1 : 0, max ? 1 : 0};
  for (int i = 0; i < out.size(); ++i)
    out.assign(i, uid(dre) == 1);
}

template<typename T, typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value, bool>::type = true>
void fill_with_random_val( std::vector<T>& out, T min, T max )
{
  std::default_random_engine dre{std::random_device()()};
  std::uniform_int_distribution<T> uid{min, max};
  for (auto &elem : out)
    elem = uid(dre);
}

template<typename T, typename std::enable_if<std::is_floating_point<T>::value, bool>::type = true>
void fill_with_random_val( std::vector<T>& out, T min, T max )
{
  std::default_random_engine dre{std::random_device()()};
  std::uniform_real_distribution<T> urd{min, max};
  for (auto &elem : out)
    elem = max - urd(dre);
}


template< typename T >
class scalar
{
  public:
    using elem_type = T;
    using host_type = elem_type;

    explicit scalar( T val )
      : _val( val )
    {}

    scalar( T min, T max )
    : _val()
    {
      fill_with_random_val(_val, min, max);
    }


    T get() const
    {
      return _val;
    }
  
    T* get_ptr()
    {
      return &_val;
    }
  
  private:
    T _val;


};

template< typename T >
class buffer_class
{
  public:
    using elem_type = T;
    using host_type = std::vector<elem_type>;

    buffer_class( const std::vector<T>& vector, bool copy_once = false )
      : _vector( vector ), _copy_once(copy_once)
    {}

    template<typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    buffer_class( const size_t& size, T min, T max, bool copy_once = false )
      : _vector( size ), _copy_once(copy_once)
    {
      fill_with_random_val(_vector, min, max);
    }

    size_t size() const
    {
      return _vector.size();
    }
  
    const T* get() const
    {
      return _vector.data();
    }
  
    const std::vector<T>& get_vector() const
    {
      return _vector;
    }

    bool copy_once() const
    {
      return _copy_once;
    }
  
  private:
    std::vector<T> _vector;
    bool           _copy_once;
};

// factory for "buffer"
template< typename T >
auto buffer( const std::vector<T>& vector )
{
  return buffer_class<T>( vector );
}

template< typename T >
auto buffer( const size_t& size )
{
  return buffer_class<T>( size );
}




template< typename... Ts >
std::tuple<Ts...> inputs( Ts... inputs )
{
  return std::tuple<Ts...>( inputs... );
}

} // namespace "data"

template<int N, typename... Ts> using NthTypeOf = typename std::tuple_element<N, std::tuple<Ts...>>::type;

namespace cf {

class kernel_info
{
  public:
    kernel_info( std::string source, std::string name = "func", std::string flags = "" )
        : _source( source ), _name( name ), _flags( flags )
    {}

    std::string source() const
    {
      return _source;
    }

    std::string name() const
    {
      return _name;
    }

    std::string flags() const
    {
      return _flags;
    }

  private:
    std::string _source;
    std::string _name;
    std::string _flags;
};

} // namespace "cf"

coordinates random_coordinates(size_t dimensionality) {
  std::default_random_engine dre{std::random_device()()};
  std::uniform_real_distribution<double> urd{0.0, 1.0};
  coordinates coords(dimensionality);
  for (auto &coord : coords)
    coord = 1.0 - urd(dre);
  return coords;
}

coordinates& clamp_coordinates_capped(coordinates& coords) {
  for( auto& coord : coords )
    coord = std::max( std::numeric_limits<double>::denorm_min(), std::min( 1.0, coord ) );
  return coords;
}
coordinates clamp_coordinates_capped(const coordinates& coords) {
  auto clamped_coords = coords;
  return clamp_coordinates_capped(clamped_coords);
}

coordinates& clamp_coordinates_mod(coordinates& coords) {
  for( auto& coord : coords ) {
    coord = std::fmod( std::abs( coord ), 1.0 );
    if (coord == 0.0)
      coord = std::numeric_limits<double>::denorm_min();
  }
  return coords;
}
coordinates clamp_coordinates_mod(const coordinates& coords) {
  auto clamped_coords = coords;
  return clamp_coordinates_mod(clamped_coords);
}

bool valid_coordinates(const coordinates& coords) {
  return std::all_of(coords.begin(), coords.end(), [](double c) { return 0.0 < c && c <= 1.0; } );
}

coordinates operator+(const coordinates& lhs, const coordinates& rhs) {
  if (lhs.size() != rhs.size())
    throw std::runtime_error("can only add equally sized coordinates");
  coordinates result = lhs;
  for (size_t i = 0; i < result.size(); ++i) {
    result[i] += rhs[i];
  }
  return result;
}
coordinates operator-(const coordinates& lhs, const coordinates& rhs) {
  if (lhs.size() != rhs.size())
    throw std::runtime_error("can only subtract equally sized coordinates");
  coordinates result = lhs;
  for (size_t i = 0; i < result.size(); ++i) {
    result[i] -= rhs[i];
  }
  return result;
}
coordinates operator*(const coordinates& lhs, double rhs) {
  coordinates result = lhs;
  for (double &i : result) {
    i *= rhs;
  }
  return result;
}

/**
 * @return current local time as ISO 8601 timestamp string
 */
std::string timestamp_str() {
  using namespace std::chrono;

  // get current time
  auto now = system_clock::now();

  // get number of milliseconds for the current second
  // (remainder after division into seconds)
  auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

  // convert to std::time_t in order to convert to std::tm (broken time)
  auto timer = system_clock::to_time_t(now);

  // convert to broken time
  std::tm bt = *std::localtime(&timer);

  std::ostringstream oss;

  oss << std::put_time(&bt, "%FT%H:%M:%S"); // HH:MM:SS
  oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

  return oss.str();
}

// comparator
template<typename T>
using comparator = std::function<bool(const T&, const T&)>;

auto equality() {
  return [](const auto& lhs, const auto& rhs) { return lhs == rhs; };
}

template<typename T>
comparator<T> absolute_difference(const T& max_difference) {
  return [max_difference](const T& lhs, const T& rhs) { return std::abs(lhs - rhs) <= max_difference; };
}

} // namespace "atf"

#endif /* helper_h */
