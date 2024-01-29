#ifndef torczon_h
#define torczon_h


#include <random>
#include <chrono>


#include "search_technique.hpp"


#define INIT_SIMPLEX_NORMALIZED_SIDE_LENGTH 0.1


namespace atf
{

class torczon : public search_technique
{
  public:
    enum torczon_state { TORC_INITIAL, TORC_REFLECTED, TORC_EXPANDED };

    using simplex = std::vector<coordinates>;

    struct torczon_simplex {
      size_t best_vertex_index = 0;
      simplex simp;
    };

    void initialize( size_t dimensionality ) override
    {
      _dimensionality = dimensionality;

      _base_simplex.simp              = initial_simplex();
      _base_simplex.best_vertex_index = 0;
      _current_simplex                = &_base_simplex;

      _current_state = TORC_INITIAL;
      _current_vertex_index = 0;
      _current_center_index = 0;
      _cost_improved = true;
      _best_cost = std::numeric_limits<cost_t>::max();
    }


    std::set<coordinates> get_next_coordinates() override
    {
      if( _current_vertex_index == _dimensionality + 1 )
        generate_next_simplex();

      return { clamp_coordinates_capped( _current_simplex->simp[_current_vertex_index] ) };
    }


    void report_costs( const std::map<coordinates, cost_t>& costs ) override
    {
      cost_t cost = costs.begin()->second;
      if( cost < _best_cost )
      {
        _best_cost                          = cost;
        _cost_improved                      = true;
        _current_simplex->best_vertex_index = _current_vertex_index;

        if( _current_state == TORC_INITIAL )
          _current_center_index = _current_vertex_index;
      }

      _current_vertex_index++;
    }


    void finalize() override
    {}

  private:
    size_t _dimensionality;

    std::default_random_engine             _rnd_generator{random_seed()};
    std::uniform_real_distribution<double> _coord_distribution{0.0, 1.0};

    double _param_expansion = 2.0;
    double _param_contraction = 0.5;

    torczon_simplex  _base_simplex;
    torczon_simplex  _test_simplex;
    torczon_simplex* _current_simplex;
    size_t           _current_vertex_index;
    size_t           _current_center_index;

    torczon_state _current_state;
    cost_t        _best_cost;
    bool          _cost_improved;


    static size_t random_seed()
    {
      return std::chrono::system_clock::now().time_since_epoch().count();
    }


    simplex initial_simplex()
    {
      static_assert( INIT_SIMPLEX_NORMALIZED_SIDE_LENGTH > 0.0, "normalized side length has to be greater than 0" );
      static_assert( INIT_SIMPLEX_NORMALIZED_SIDE_LENGTH <= 0.5, "normalized side length has to be less than or equal to 0.5" );

      simplex simp;

      // generate random base vertex
      auto base_vertex_coords = random_coordinates(_dimensionality);

      // construct simplex from base vertex
      simp.push_back( ( coordinates( base_vertex_coords ) ) );
      for( size_t i = 0; i < _dimensionality; i++ )
      {
        auto v = base_vertex_coords;

        if( v[ i ] <= 0.5 )
          v[ i ] += INIT_SIMPLEX_NORMALIZED_SIDE_LENGTH;
        else
          v[ i ] -= INIT_SIMPLEX_NORMALIZED_SIDE_LENGTH;

        simp.push_back( coordinates( v ) );
      }

      return simp;
    }


    simplex expand_base_simplex( double factor ) const
    {
      simplex expanded_simplex;

      const coordinates& center = _base_simplex.simp[ _current_center_index ];
      for( const coordinates& v : _base_simplex.simp )
      {
        coordinates new_vertex = clamp_coordinates_capped( center * ( 1 - factor ) + v * factor );
        expanded_simplex.push_back( new_vertex );
      }

      return expanded_simplex;
    }


    simplex reflect_base_simplex() const
    {
      return expand_base_simplex( -1 );
    }


    simplex expand_base_simplex() const
    {
      return expand_base_simplex( _param_expansion );
    }


    simplex contract_base_simplex() const
    {
      return expand_base_simplex( _param_contraction );
    }


    void switch_state( torczon_state new_state )
    {
      _current_state        = new_state;
      _current_vertex_index = 0;
      _cost_improved        = false;
    }


    void generate_next_simplex()
    {
      switch (_current_state)
      {
        case TORC_INITIAL:
        {
          _test_simplex.simp              = reflect_base_simplex();
          _test_simplex.best_vertex_index = 0;
          _current_simplex                = &_test_simplex;

          switch_state( TORC_REFLECTED );

          break;
        }

        case TORC_REFLECTED:
        {
          if( _cost_improved )
          {
            _base_simplex                   = _test_simplex;
            _test_simplex.simp              = expand_base_simplex();
            _test_simplex.best_vertex_index = 0;
            _current_simplex                = &_test_simplex;

            switch_state( TORC_EXPANDED );
          }
          else
          {
            _base_simplex.simp              = contract_base_simplex();
            _base_simplex.best_vertex_index = 0;
            _current_simplex                = &_base_simplex;

            _best_cost            = std::numeric_limits<cost_t>::max();
            _current_center_index = 0;

            switch_state( TORC_INITIAL );
          }

          break;
        }

        case TORC_EXPANDED:
        {
          if( _cost_improved )
            _base_simplex = _test_simplex;

          _current_center_index           = _base_simplex.best_vertex_index;
          _test_simplex.simp              = reflect_base_simplex();
          _test_simplex.best_vertex_index = 0;
          _current_simplex                = &_test_simplex;

          switch_state( TORC_REFLECTED );

          break;
        }

        default:
          throw std::runtime_error( "Invalid algorithm state" );
      }
    }

};

} // namespace "atf"

#endif /* torczon_h */
