#ifndef pattern_search_h
#define pattern_search_h

#include "search_technique.hpp"

namespace atf
{

class pattern_search : public search_technique
{
  public:
    void initialize( size_t dimensionality ) override
    {
      _dimensionality = dimensionality;
      _base = random_coordinates(_dimensionality);
      _trigger = false;
      _step_size = 0.1;
      _current_parameter = 0;
      _current_state = INITIALIZATION;
    }

    std::set<coordinates> get_next_coordinates() override
    {
      switch (_current_state) {
        case INITIALIZATION: {
          _exploratory_coordinates = _base;
          _pattern_coordinates = _base;
          return {clamp_coordinates_capped( _base ) };
        }
        case EXPLORATORY_PLUS: {
          coordinates p = _exploratory_coordinates;
          p[_current_parameter] += _step_size;
          return {clamp_coordinates_capped( p ) };
        }
        case EXPLORATORY_MINUS: {
          coordinates p = _exploratory_coordinates;
          if(_trigger) {
            p[_current_parameter] -= 2*_step_size;
          }
          else {
            p[_current_parameter] -= _step_size;
          }
          return {clamp_coordinates_capped( p ) };
        }
        case PATTERN: {
          return {clamp_coordinates_capped( _pattern_coordinates ) };
        }
        default:
          throw std::runtime_error( "Invalid algorithm state" );
      }
    }

    void report_costs( const std::map<coordinates, cost_t>& costs ) override
    {
      cost_t cost = costs.begin()->second;
      switch(_current_state)
      {
        case INITIALIZATION: {
          if(cost == std::numeric_limits<atf::cost_t>::max()) {
            _base = random_coordinates(_dimensionality);
            break;
          }
          _base_fitness = cost;
          _exploratory_coordinates_fitness = cost;
          _pattern_coordinates_fitness = cost;
          _current_state = EXPLORATORY_PLUS;
          break;
        }
        case EXPLORATORY_PLUS: {
          if (cost < _exploratory_coordinates_fitness) {
            _exploratory_coordinates[_current_parameter] += _step_size;
            _exploratory_coordinates = clamp_coordinates_mod(_exploratory_coordinates);
            _exploratory_coordinates_fitness = cost;
            _trigger = true;
          }
          _current_state = EXPLORATORY_MINUS;
          break;
        }
        case EXPLORATORY_MINUS: {
          if (cost < _exploratory_coordinates_fitness) {
            if(_trigger) {
              _exploratory_coordinates[_current_parameter] -= 2*_step_size;
            }
            else {
              _exploratory_coordinates[_current_parameter] -= _step_size;
            }
            _exploratory_coordinates = clamp_coordinates_mod(_exploratory_coordinates);
            _exploratory_coordinates_fitness = cost;
          }
          _trigger = false;
          ++_current_parameter;

          if (_current_parameter == _base.size()) {
            if (_exploratory_coordinates_fitness < _pattern_coordinates_fitness) {
              _pattern_coordinates = clamp_coordinates_mod(
                  _exploratory_coordinates + (_exploratory_coordinates - _base)
              );
              _base = _exploratory_coordinates;
              _base_fitness = _exploratory_coordinates_fitness;
              _exploratory_coordinates = _pattern_coordinates;
              _current_state = PATTERN;
            } else {
              _exploratory_coordinates = _base;
              _pattern_coordinates = _base;
              _pattern_coordinates_fitness = _base_fitness;
              _exploratory_coordinates_fitness = _base_fitness;
              decrement_step_size();
              _current_state = EXPLORATORY_PLUS;
            }
            _current_parameter = 0;
          } else {
            _current_state = EXPLORATORY_PLUS;
          }
          break;
        }
        case PATTERN: {
          _pattern_coordinates_fitness = cost;
          _exploratory_coordinates_fitness = cost;
          _current_state = EXPLORATORY_PLUS;
          break;
        }
        default:
          throw std::runtime_error( "Invalid algorithm state" );
      }
    }

    /**
     * \brief Finalizes the pattern search technique.
     */
    void finalize() override
    {}

  private:
    /** state to indicate what do do next */
    enum State{
      INITIALIZATION,
      EXPLORATORY_PLUS,
      EXPLORATORY_MINUS,
      PATTERN
    };

    /** dimensionality of search space */
    size_t      _dimensionality;
    /** base coordinates to go back to and its fitness */
    coordinates _base;
    double      _base_fitness;
    /** coordinates that moves through the search space and its fitness */
    coordinates _exploratory_coordinates;
    double      _exploratory_coordinates_fitness;
    /** coordinates and its fitness after pattern move */
    coordinates _pattern_coordinates;
    double      _pattern_coordinates_fitness;
    /** trigger to flag if Parameter has increased */
    bool        _trigger;
    /** index of current parameter */
    size_t      _current_parameter;
    /** current step size */
    double      _step_size;
    /** current state */
    State       _current_state;

    void decrement_step_size()
    {
      _step_size *= 0.5;
    }
};

} // namespace "atf"

#endif /* pattern_search_h */
