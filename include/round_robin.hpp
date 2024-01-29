#ifndef round_robin_hpp
#define round_robin_hpp

#include <deque>
#include <algorithm>
#include <numeric>
#include <random>
#include <utility>

#include "search_technique.hpp"

namespace atf
{

template< typename... Ts >
class round_robin_class : public search_technique
{
public:
    explicit round_robin_class( std::tuple<Ts...> techniques )
        : _techniques( std::move(techniques) ), _current_technique_index( 0 )
    {}


    void initialize(size_t dimensionality) override
    {
      initialize_impl( dimensionality, std::make_index_sequence<sizeof...(Ts)>{} );
    }


    std::set<coordinates> get_next_coordinates() override
    {
      return get_next_coordinates_impl( std::make_index_sequence<sizeof...(Ts)>{} );
    }


    void report_costs( const std::map<coordinates, cost_t>& costs ) override
    {
      report_costs_impl( costs, std::make_index_sequence<sizeof...(Ts)>{} );

      _current_technique_index = ( _current_technique_index + 1 ) % sizeof...(Ts);
    }


    void finalize() override
    {
      finalize_impl( std::make_index_sequence<sizeof...(Ts)>{} );
    }


private:
    std::tuple<Ts...> _techniques;
    size_t            _current_technique_index;

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
auto round_robin( const std::tuple<Ts...>& techniques ) {
  return round_robin_class<Ts...>( techniques );
}

auto round_robin() {
  return round_robin( std::make_tuple( simulated_annealing()    ,
                                       differential_evolution() ,
                                       particle_swarm()         ,
                                       pattern_search()         ,
                                       torczon()                ) );
}

} // namespace "atf"



#endif /* round_robin_hpp */
