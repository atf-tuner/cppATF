#ifndef simulated_annealing_h
#define simulated_annealing_h

#include <map>
#include <utility>
#include <random>
#include <cmath>
#include <limits>

#include "search_technique.hpp"

namespace atf {

class simulated_annealing : public search_technique
{
  public:
    void initialize( size_t dimensionality ) override
    {
      _dimensionality = dimensionality;
      _current_state = INITIALIZATION;
      _time = 0;
      while(interp_steps.size() < temps.size() - 1)
      {
        interp_steps.push_back(_default_interp_steps);
      }

      for(size_t t = 0; t < temps.size() - 1; ++t)
      {
        for(int steps = interp_steps[t]; steps > 0; --steps)
        {
          _schedule.push_back(interp(temps[t+1], temps[t], static_cast<double>(steps)/interp_steps[t]));
        }
      }
      _schedule.push_back(temps.back());
      _max_time = static_cast<int>(_schedule.size()) - 1;
    }

    std::set<coordinates> get_next_coordinates() override
    {
      switch(_current_state) {
        case INITIALIZATION: {
          _current_parameter = 0;
          _temp = _schedule.at(std::min(_time, _max_time));
          _step_size = get_step_size(_time, _temp);
          _current_coordinates = random_coordinates(_dimensionality);
          _neighbours.emplace_back(_current_coordinates, 0.0);
          return { clamp_coordinates_capped( _neighbours.front().first ) };
        }
        case EXPLORE_PLUS: {
          if(_current_coordinates[_current_parameter] < 1.0f) {
            _neighbours.emplace_back(_current_coordinates, 0.0);
            _neighbours.back().first[_current_parameter] += _step_size*random();
            if(_current_coordinates[_current_parameter] <= 0.0f) {
              _current_state = EXPLORE_MINUS;
            }
            return { clamp_coordinates_capped( _neighbours.back().first ) };
          }
          _current_state = EXPLORE_MINUS;
          _neighbours.emplace_back(_current_coordinates, 0.0);
          _neighbours.back().first[_current_parameter] -= _step_size*random();
          return { clamp_coordinates_capped( _neighbours.back().first ) };
        }
        case EXPLORE_MINUS: {
          _neighbours.emplace_back(_current_coordinates, 0.0);
          _neighbours.back().first[_current_parameter] -= _step_size*random();
          return { clamp_coordinates_capped( _neighbours.back().first ) };
        }
        default:
          throw std::runtime_error( "Invalid algorithm state" );
      }
    }

    void report_costs( const std::map<coordinates, cost_t>& costs ) override
    {
      atf::cost_t cost = costs.begin()->second;
      switch(_current_state) {
        case INITIALIZATION: {
          _neighbours.front().second = cost;
          _best_coordinates = costs.begin()->first;
          _best_result = cost;
          _current_state = EXPLORE_PLUS;
          break;
        }
        case EXPLORE_PLUS: {
          _neighbours.back().second = cost;
          if(_neighbours.back().second < _best_result) {
            _best_coordinates = _neighbours.back().first;
            _best_result = _neighbours.back().second;
          }
          _current_state = EXPLORE_MINUS;
          break;
        }
        case EXPLORE_MINUS: {
          _neighbours.back().second = cost;
          if(_neighbours.back().second < _best_result) {
            _best_coordinates = _neighbours.back().first;
            _best_result = _neighbours.back().second;
          }
          ++_current_parameter;
          if(_current_parameter == _dimensionality) {
            _current_parameter = 0;
            atf::cost_t current_result;
            while(true) {
              if(_neighbours.empty()) {
                _current_coordinates = _best_coordinates;
                current_result = _best_result;
                break;
              }
              std::uniform_int_distribution<> uid{0, static_cast<int>(_neighbours.size()) - 1};
              int candidate = uid(_dre);
              if (random() < AcceptanceFunction(1.0f ,
                                                relative(_neighbours.at(candidate).second,
                                                         _best_result),
                                                _temp)) {
                _current_coordinates = _neighbours.at(candidate).first;
                current_result = _neighbours.at(candidate).second;
                break;
              }
              _neighbours.erase(_neighbours.begin() + candidate);
            }
            ++_time;
            if(_time > _max_time) {
              _time = _time - _max_time;
            }
            _temp = _schedule.at(std::min(_time, _max_time));
            _step_size = get_step_size(_time, _temp);
            _neighbours.clear();
            _neighbours.emplace_back(_current_coordinates, current_result);
            _current_state = EXPLORE_PLUS;
          }
          else {
            _current_state = EXPLORE_PLUS;
          }
          break;
        }
        default:
          throw std::runtime_error( "Invalid algorithm state" );
      }
    }

    void finalize() override
    {}


  private:
    /** holds the default number of steps to interpolate */
    const int _default_interp_steps = 100;
    /** holds a list of temperatures to interpolate between */
    const std::vector<double> temps = {30, 0};
    /** holds a set of numbers that determine how to interpolate between temps elements */
    std::vector<int> interp_steps;

    /** state to indicate what to do next */
    enum State{
      INITIALIZATION,
      EXPLORE_PLUS,
      EXPLORE_MINUS
    };
    /** current state */
    State                                       _current_state;
    /** number of determined next configurations/time */
    int                                         _time;
    /** maximum number of configurations within this cooling period/maximum cooling time */
    int                                         _max_time;
    /** indicates the current parameter to mutate*/
    size_t                                      _current_parameter;
    /** dimensionality of coordinate space */
    size_t                                      _dimensionality;
    /** holds the current best result found */
    cost_t                                      _best_result;
    /** holds the current temperature */
    double                                      _temp;
    /** holds the current step size range */
    double                                      _step_size;
    /** holds the current coordinates */
    coordinates                                 _current_coordinates;
    /** holds the best coordinates found yet */
    coordinates                                 _best_coordinates;
    /** the temperature schedule */
    std::vector<double>                         _schedule;
    /** vector that holds all potentially next points */
    std::vector<std::pair<coordinates, cost_t>> _neighbours;
    /**  the random engine and distribution */
    std::default_random_engine _dre{std::random_device()()};
    std::uniform_real_distribution<double>      _urd{0.0, 1.0};


    double random()
    {
      return _urd(_dre);
    }


    static double interp(double a, double b, double t)
    {
      if(t < 0.0 || t > 1.0)
      {
        throw std::invalid_argument{"t has to be in [0,1]"};
      }
      return a + t*(b-a);
    }


    static double get_step_size(int time, double temp)
    {
      return exp(-(20.0 + static_cast<double>(time)/100.0)/(temp + 1.0));
    }


    static double AcceptanceFunction(double e, double e_new, double temp)
    {
      if(e >= e_new) {
        return 1.0;
      }
      if(temp == 0) {
        return 0.0;
      }
      if(50*(e_new-e)/temp > 10) {
        return 0.0;
      }
      return exp(50.0*(e-e_new)/temp);
    }


    static double relative(double result1, double result2)
    {
      if(result2 == 0) {
        return result1 * std::numeric_limits<double>::infinity();
      }
      return result1/result2;
    }
};

} // namespace "atf"

#endif /* simulated_annealing_h */
