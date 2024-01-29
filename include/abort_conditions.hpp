
#ifndef abort_conditions_h
#define abort_conditions_h


#include <chrono>
#include <vector>
#include <memory>
#include <iostream>

#include "abort_condition.hpp"

namespace atf
{

namespace cond
{

class or_class : public atf::abort_condition
{
  public:
    template< typename... Ts >
    or_class( const Ts&... conditions )
    {
      add( conditions... );
    }

  
    bool stop( const tuning_status& status )
    {
      for( const auto& cond : s )
        if( cond->stop( status ) )
          return true;

      return false;
    }

  private:
    std::vector<std::shared_ptr<abort_condition>> s;

    // IS
    template< typename T, typename... Ts, std::enable_if_t< std::is_base_of<abort_condition, T>::value >* = nullptr >
    void add( const T& condition, const Ts&... conditions )
    {
      auto res = std::make_shared<T>( condition );
      
      s.push_back( res );
      
      add( conditions... );
    }


    // IA
    void add()
    {}
};

template< typename T_lhs, typename T_rhs, std::enable_if_t<std::is_base_of<abort_condition, T_lhs>::value && std::is_base_of<abort_condition, T_rhs>::value >* = nullptr >
or_class operator||( const T_lhs& lhs, const T_rhs& rhs )
{
  return or_class( lhs, rhs );
}

class and_class : public abort_condition
{
  public:
    template< typename... Ts >
    and_class( const Ts&... conditions )
    {
      add( conditions... );
    }

    
    bool stop( const tuning_status& status )
    {
      for ( const auto& cond : s )
        if ( cond->stop(status))
          return false;

      return true;
    }

  private:
    std::vector<std::shared_ptr<abort_condition>> s;

    // IS
    template< typename T, typename... Ts, std::enable_if_t< std::is_base_of<abort_condition, T>::value >* = nullptr >
    void add( const T& condition, const Ts&... conditions )
    {
      auto res = std::make_shared<T>(condition);
      
      s.push_back( res );
      
      add( conditions... );
    }


    // IA
    void add()
    {}
};

template< typename T_lhs, typename T_rhs, std::enable_if_t<std::is_base_of<abort_condition, T_lhs>::value && std::is_base_of<abort_condition, T_rhs>::value >* = nullptr >
and_class operator&&( const T_lhs& lhs, const T_rhs& rhs )
{
  return and_class( lhs, rhs );
}



class evaluations : public abort_condition
{
  public:
    evaluations( const size_t& num_evaluations )
            : _num_evaluations( num_evaluations )
    {}
  
    bool stop( const tuning_status& status )
    {
      auto number_of_evaluated_configs = status.number_of_evaluated_configs();
      return number_of_evaluated_configs >= _num_evaluations;
    }
  private:
    size_t _num_evaluations;
};


class valid_evaluations : public abort_condition
{
  public:
    valid_evaluations( const size_t& num_evaluations )
      : _num_evaluations( num_evaluations )
    {}
  
    bool stop( const tuning_status& status )
    {
      auto number_of_valid_evaluated_configs = status.number_of_valid_configs();
      return number_of_valid_evaluated_configs >= _num_evaluations;
    }
  private:
    size_t _num_evaluations;
};


class speedup : public abort_condition
{
    enum DurationType
    {
      NUM_CONFIGS,
      TIME
    };
  
  public:
    speedup( const double& speedup, const size_t& num_configs = 1            , const bool& only_valid_configs = true )
      : _speedup( speedup ), _duration(), _num_configs( num_configs ), _type( DurationType::NUM_CONFIGS ), _only_valid_configs( only_valid_configs )
    {}
    speedup( const double& speedup, const std::chrono::milliseconds& duration, const bool& only_valid_configs = true )
      : _speedup( speedup ), _duration( duration ), _num_configs(), _type( DurationType::TIME ), _only_valid_configs( only_valid_configs )
    {}
  
    ~speedup()
    {
    }
  
    bool stop( const tuning_status& status )
    {
      if( _only_valid_configs && status.min_cost() == std::numeric_limits<size_t>::max() );
      else
        _verbose_history.emplace_back( status.min_cost() );

      // two cases
      if( _type == NUM_CONFIGS )
      {
        // starting phase
        if( _verbose_history.size() < _num_configs )
          return false;
        else
        {
          auto last_best_result = _verbose_history[ _verbose_history.size() - _num_configs ];
          auto best_result      = _verbose_history.back();
          auto speedup          = last_best_result / best_result;

                std::cout << "last best result: " << last_best_result << std::endl;
          std::cout << "best result:      " << best_result      << std::endl;
          std::cout << "Speedup:          " << speedup          << std::endl << std::endl;

          return speedup <= _speedup ;
        }
      }

      else if( _type == TIME )
      {
        assert( false );   }

      assert( false ); // should never be reached
      return true;
    }
  private:
    double                    _speedup;
    std::chrono::milliseconds _duration;
    size_t                    _num_configs;
    DurationType              _type;
    std::vector<cost_t>         _verbose_history;
    bool                      _only_valid_configs;
};


template< typename duration_t >
class duration : public abort_condition
{
  public:
    duration( size_t duration )
      : _duration( duration )
    {}
  
  
    bool stop( const tuning_status& status )
    {
      auto current_tuning_time = std::chrono::steady_clock::now() - status.tuning_start_time();
      return current_tuning_time > _duration;
    }
  private:
    duration_t _duration;
};


class result : public abort_condition
{
  public:
    result( size_t result )
      : _result( result )
    {}

    bool stop( const tuning_status& status )
    {
      auto current_best_result = status.min_cost();
      return current_best_result <= _result;
    }
  private:
    size_t _result;
};




} // namespace "cond"

} // namespace "atf"


#endif /* abort_conditions_h */
