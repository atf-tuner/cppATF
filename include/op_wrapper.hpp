
#ifndef op_wrapper_h
#define op_wrapper_h


#include "helper.hpp"


namespace atf {

template< typename T >
void print_type()
{
  std::cout << "unknown type" << std::endl;
}

template<>
void print_type<int>()
{
  std::cout << "int" << std::endl;
}

template<>
void print_type<size_t>()
{
  std::cout << "size_t" << std::endl;
}


template<>
void print_type<bool>()
{
  std::cout << "bool" << std::endl;
}


template< typename T_lhs, typename T_rhs,
          typename callable
        >
class op_wrapper_class
{
    using T_lhs_ref_free = typename std::remove_reference<T_lhs>::type;
    using T_rhs_ref_free = typename std::remove_reference<T_rhs>::type;
  
    static const bool T_lhs_is_fundamental = std::is_fundamental< T_lhs_ref_free >::value;
    static const bool T_rhs_is_fundamental = std::is_fundamental< T_rhs_ref_free >::value;
  
    static const bool hard_copy_lhs = T_lhs_is_fundamental || std::is_rvalue_reference< T_lhs >::value;
    static const bool hard_copy_rhs = T_rhs_is_fundamental || std::is_rvalue_reference< T_rhs >::value;
  
    using T_lhs_save_type = std::conditional_t< hard_copy_lhs, T_lhs_ref_free, T_lhs_ref_free& >;
    using T_rhs_save_type = std::conditional_t< hard_copy_rhs, T_rhs_ref_free, T_rhs_ref_free& >;
  
    using T_lhs_scalar_type = typename std::remove_reference< typename std::conditional_t< T_lhs_is_fundamental, eval_t<T_lhs>, casted_eval_t<T_lhs> >::type >::type;
    using T_rhs_scalar_type = typename std::remove_reference< typename std::conditional_t< T_rhs_is_fundamental, eval_t<T_rhs>, casted_eval_t<T_rhs> >::type >::type;

  private:
    T_lhs_save_type _lhs;     T_rhs_save_type _rhs;
    callable        _func;


  public:
        op_wrapper_class( T_lhs lhs, T_rhs rhs, callable func )
      : _lhs( lhs ), _rhs( rhs ), _func( func )
    {
    }



    using T_res = typename std::result_of<callable(T_lhs_scalar_type, T_rhs_scalar_type)>::type;
  
    auto cast() const
    {
      return operator T_res();
    }
  
    operator T_res() const
    {
      auto lhs = static_cast<T_lhs_scalar_type>( _lhs );
      auto rhs = static_cast<T_rhs_scalar_type>( _rhs );
      
      return _func( lhs, rhs );
    }
};


// default
template< typename T_lhs_hlpr, typename T_rhs_hlpr, typename callable >
auto op_wrapper( T_lhs_hlpr&& lhs, T_rhs_hlpr&& rhs, callable func )
{
  return op_wrapper_class<T_lhs_hlpr&&, T_rhs_hlpr&&, callable>( std::forward<T_lhs_hlpr>(lhs), std::forward<T_rhs_hlpr>(rhs), func);
}

class tp_int_expression {
  public:
    // implicit conversions
    tp_int_expression(int value) {
      _evaluator = [value] () {
        return value;
      };
    }
    template< typename range_t, typename callable >
    tp_int_expression(const atf::tp_t<int, range_t, callable>& tp) {
      _evaluator = [&tp] () {
        return tp.cast();
      };
    }
    template< typename range_t, typename callable >
    tp_int_expression(const atf::tp_t<size_t, range_t, callable>& tp) {
      _evaluator = [&tp] () {
        return static_cast<int>(tp.cast());
      };
    }
    template<typename T_lhs, typename T_rhs, typename callable>
    tp_int_expression(atf::op_wrapper_class<T_lhs, T_rhs, callable>&& op_wrapper) {
      _evaluator = [op_wrapper] () {
        return static_cast<int>(op_wrapper.cast());
      };
    }

    int evaluate() {
      return _evaluator();
    }
  private:
    std::function<int()> _evaluator;
};

}
#endif /* op_wrapper_h */
