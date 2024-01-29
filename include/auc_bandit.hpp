#ifndef auc_bandit_hpp
#define auc_bandit_hpp

#include <deque>
#include <algorithm>
#include <numeric>
#include <random>
#include <utility>

#include "search_technique.hpp"

namespace atf
{

template< typename... Ts >
class auc_bandit_class : public search_technique
{
public:
    static constexpr double DEFAULT_C = 0.05;
    static constexpr size_t DEFAULT_WINDOW_SIZE = 500;

    struct history_entry {
      size_t technique_index;
      bool   cost_has_improved;
    };


    explicit auc_bandit_class( std::tuple<Ts...> techniques, double c = DEFAULT_C, size_t window_size = DEFAULT_WINDOW_SIZE )
        : _c( c ), _window_size( window_size ), _rand_generator( random_seed() ), _techniques( std::move(techniques) ), _current_technique_index( 0 ),
          _current_best_cost( std::numeric_limits<cost_t>::max() ), _history(), _uses(sizeof...(Ts) ), _raw_auc(sizeof...(Ts) ), _decay(sizeof...(Ts) ),
          _log_file()
    {}
    auc_bandit_class(const auc_bandit_class<Ts...>& other):
        _c( other._c ),
        _window_size( other._window_size ),
        _rand_generator( other._rand_generator ),
        _techniques( other._techniques ),
        _current_technique_index( other._current_technique_index ),
        _current_best_cost( other._current_best_cost ),
        _history( other._history ),
        _uses( other._uses ),
        _raw_auc( other._raw_auc ),
        _decay( other._decay ),
        _log_file()
    {};


    void initialize(size_t dimensionality) override
    {
      _log_file.open("auc_bandit_log.csv", std::ios::out | std::ios::trunc);
      _log_file << "search_technique_index";
      initialize_impl( dimensionality, std::make_index_sequence<sizeof...(Ts)>{} );
    }


    std::set<coordinates> get_next_coordinates() override
    {
      _current_technique_index = get_best_technique_index();
      _log_file << std::endl << _current_technique_index;
      return get_next_coordinates_impl( std::make_index_sequence<sizeof...(Ts)>{} );
    }


    void report_costs( const std::map<coordinates, cost_t>& costs ) override
    {
      report_costs_impl( costs, std::make_index_sequence<sizeof...(Ts)>{} );

      bool cost_has_improved = false;
      auto min_cost = std::numeric_limits<cost_t>::max();
      for (const auto& cost : costs)
        min_cost = std::min(min_cost, cost.second);
      if( min_cost < _current_best_cost )
      {
        _current_best_cost = min_cost;
        cost_has_improved = true;
      }

      history_push( _current_technique_index, cost_has_improved );
    }


    void finalize() override
    {
      finalize_impl( std::make_index_sequence<sizeof...(Ts)>{} );
      _log_file.close();
    }


private:
    std::ofstream _log_file;

    double _c;
    size_t _window_size;

    std::default_random_engine _rand_generator;

    std::tuple<Ts...> _techniques;
    size_t            _current_technique_index;
    cost_t            _current_best_cost;

    std::deque<history_entry> _history;
    std::vector<size_t>       _uses;
    std::vector<size_t>       _raw_auc;
    std::vector<size_t>       _decay;


    size_t random_seed() const
    {
      return std::chrono::system_clock::now().time_since_epoch().count();
    }


    void history_push( size_t technique_index, bool cost_has_improved )
    {
      // remove oldest entry if history is full
      if( _history.size() == _window_size )
      {
        auto oldest_technique = _history.front();

        _uses[ oldest_technique.technique_index ]--;
        _raw_auc[ oldest_technique.technique_index ] -= _decay[ oldest_technique.technique_index ];
        if( oldest_technique.cost_has_improved )
          _decay[ oldest_technique.technique_index ]--;

        _history.pop_front();
      }

      _uses[ technique_index ]++;
      if( cost_has_improved )
      {
        _raw_auc[ technique_index ] += _uses[ technique_index ];
        _decay[ technique_index ]   += 1;
      }

      _history.push_back( { technique_index, cost_has_improved } );
    }


    double calculate_auc( size_t technique_index )
    {
      auto uses = _uses[ technique_index ];
      if( uses > 0 )
        return _raw_auc[ technique_index ] * 2.0 / ( uses * ( uses + 1.0 ) );
      else
        return 0.0;
    }


    double calculate_exploration_value( size_t technique_index )
    {
      if( _uses[ technique_index ] > 0 )
        return std::sqrt( 2.0 * std::log2( _history.size() ) / _uses[ technique_index ] );
      else
        return std::numeric_limits<double>::infinity();
    }


    double calculate_score( size_t technique_index )
    {
      return calculate_auc( technique_index ) + _c * calculate_exploration_value( technique_index );
    }


    size_t get_best_technique_index()
    {
      std::vector<size_t> indices( sizeof...(Ts) );
      std::iota( indices.begin(), indices.end(), 0 );

      // randomize order of techniques with equal score
      std::shuffle( indices.begin(), indices.end(), _rand_generator );

      return *std::max_element( indices.begin(), indices.end(), [&]( size_t i_1, size_t i_2 ) {
        return calculate_score( i_1 ) < calculate_score( i_2 );
      } );
    }

    template<size_t... Is>
    void initialize_impl( size_t dimensionality, std::index_sequence<Is...> ) {
        initialize_impl( dimensionality, std::get<Is>(_techniques)... );
    }
    template<typename T, typename... ARGS>
    void initialize_impl( size_t dimensionality, T& technique, ARGS&... techniques ) {
        technique.initialize( dimensionality );
        initialize_impl( dimensionality, techniques... );
    }
    void initialize_impl( size_t dimensionality ) {
    }

    template<size_t... Is>
    std::set<coordinates> get_next_coordinates_impl( std::index_sequence<Is...> ) {
        return get_next_coordinates_impl( 0, std::get<Is>(_techniques)... );
    }
    template<typename T, typename... ARGS>
    std::set<coordinates> get_next_coordinates_impl( size_t index, T& technique, ARGS&... techniques ) {
        if (index == _current_technique_index)
            return technique.get_next_coordinates();
        else
            return get_next_coordinates_impl( index + 1, techniques... );
    }
    std::set<coordinates> get_next_coordinates_impl(size_t index) {
        assert(false && "should never be reached");
    }

    template<size_t... Is>
    void report_costs_impl(const std::map<coordinates, cost_t>& costs, std::index_sequence<Is...> ) {
        report_costs_impl( costs, 0, std::get<Is>(_techniques)... );
    }
    template<typename T, typename... ARGS>
    void report_costs_impl(const std::map<coordinates, cost_t>& costs, size_t index, T& technique, ARGS&... techniques ) {
        if (index == _current_technique_index)
            technique.report_costs(costs);
        else
            report_costs_impl( costs, index + 1, techniques... );
    }
    void report_costs_impl(const std::map<coordinates, cost_t>& costs, size_t index) {
        assert(false && "should never be reached");
    }

    template<size_t... Is>
    void finalize_impl( std::index_sequence<Is...> ) {
        finalize_impl( std::get<Is>(_techniques)... );
    }
    template<typename T, typename... ARGS>
    void finalize_impl( T& technique, ARGS&... techniques ) {
        technique.finalize();
        finalize_impl( techniques... );
    }
    void finalize_impl() {
    }
};

template<typename... Ts>
auto auc_bandit( const std::tuple<Ts...>& techniques, double c = auc_bandit_class<Ts...>::DEFAULT_C, size_t window_size = auc_bandit_class<Ts...>::DEFAULT_WINDOW_SIZE ) {
  return auc_bandit_class<Ts...>( techniques, c, window_size);
}

auto auc_bandit( double c = auc_bandit_class<>::DEFAULT_C, size_t window_size = auc_bandit_class<>::DEFAULT_WINDOW_SIZE ) {
  return auc_bandit( std::make_tuple( simulated_annealing()    ,
                                      pattern_search()         ,
                                      torczon()                ) ,
                     c,
                     window_size );
}

} // namespace "atf"



#endif /* auc_bandit_hpp */
