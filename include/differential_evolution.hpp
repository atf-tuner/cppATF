#ifndef differential_evolution_h
#define differential_evolution_h

#include <random>

#include "search_technique.hpp"

/** number of vectors of the population with a minimum of 4 */
#define NUM_VECTORS     30
/** number of vectors to create the donor vector */
#define NUM_MUT_VECTORS 3
/** mutation factor which scale the donor vector */
#define F_VAL           0.7      // 0.4 - 1.0
/** Crossover-Rate to combine the donor vector with the current vector */
#define CR              0.2      // 0.0 - 1.0
/** number of retries to calculate a new trial vector if the current one isn't in the search space */
#define INVALID_RETRIES 1

namespace atf
{

class differential_evolution : public search_technique
{
  public:
    void initialize( size_t dimensionality ) override
    {
      _dimensionality = dimensionality;

      _generator = std::default_random_engine(random_seed());
      _current_vec = 0;

      population_init();
      _trial_vector = random_coordinates(_dimensionality);
    }

    std::set<coordinates> get_next_coordinates() override
    {
      if(_population_costs.at(_current_vec) == -1){
        return { clamp_coordinates_capped( _vector_population.at(_current_vec) ) };
      } else {
        setTrialVector();
        return { clamp_coordinates_capped( _trial_vector ) };
      }
    }

    void report_costs( const std::map<coordinates, cost_t>& costs ) override
    {
      cost_t cost = costs.begin()->second;
      if(_population_costs.at(_current_vec) == -1){
        if(cost == std::numeric_limits<atf::cost_t>::max()){
          _vector_population.at(_current_vec) = random_coordinates(_dimensionality);
        }
        else
          _population_costs.at(_current_vec) = cost;
      }
      else if(cost <= _population_costs.at(_current_vec)){
        _vector_population.at(_current_vec) = _trial_vector;
        _population_costs.at(_current_vec) = cost;
      }

      if(_current_vec < NUM_VECTORS -1)
        _current_vec++;
      else{
        _current_vec = 0;           //Next generation
      }
    }

    void finalize() override
    {}

  private:
    size_t                                 _dimensionality;
    /** generator to produce random values */
    std::default_random_engine             _generator;
    /** vector of points from population */
    std::vector<coordinates>               _vector_population;
    /** trial vector */
    coordinates                            _trial_vector;
    /** vector of costs from each point of population */
    std::vector<cost_t>                    _population_costs;
    /** counter of the vectors of population */
    size_t                                 _current_vec;

    static unsigned int random_seed()
    {
      return static_cast<unsigned int>( std::chrono::system_clock::now().time_since_epoch().count() );
    }

    void population_init()
    {
      for(int i = 0; i < NUM_VECTORS; i++) {
        _vector_population.push_back(random_coordinates(_dimensionality));
        _population_costs.push_back(-1);
      }
    }

    int* random_vectors() {
      std::uniform_int_distribution<int>  mutation_distribution = std::uniform_int_distribution<int>( 0, static_cast<int>(NUM_VECTORS -1) );
      int *vecs = new int[NUM_MUT_VECTORS];
      for(int i = 0; i < NUM_MUT_VECTORS; i++){
        vecs[i] = mutation_distribution(_generator);
        for(int j = 0; j<i; j++){
          if( (vecs[i] == vecs[j] && i != j) || vecs[i] == _current_vec){
            vecs[i] = mutation_distribution(_generator);
            j=-1;
          }
        }
      }
      return vecs;
    }

    void setTrialVector() {
      int random_param, *mutation_vec_indizes,loop_count=0;
      std::uniform_int_distribution<int> recombi_distribution = std::uniform_int_distribution<int>(0, static_cast<int>(_dimensionality - 1));
      std::uniform_real_distribution<double> cr_distribution = std::uniform_real_distribution<double>(0,static_cast<double>( 1.0 ));

      do {
        loop_count++;
        random_param = recombi_distribution(_generator);
        mutation_vec_indizes = random_vectors();

        for (int i = 0; i < _dimensionality; i++) {
          if (cr_distribution(_generator) <= CR || i == random_param)
            _trial_vector[i] = getDonorVector(i, mutation_vec_indizes);
          else
            _trial_vector[i] = _vector_population.at(_current_vec)[i];
        }

      } while (! valid_coordinates(_trial_vector) && loop_count < INVALID_RETRIES);
      if(! valid_coordinates(_trial_vector))
        clamp_coordinates_mod(_trial_vector);
    }

    double getDonorVector (int param, int *mutation_vec_indizes) const {
#if 1
      return _vector_population.at(mutation_vec_indizes[0])[param]
             + F_VAL * (_vector_population.at(mutation_vec_indizes[1])[param] -
                        _vector_population.at(mutation_vec_indizes[2])[param]);
#elif 0
      auto result = std::min_element(std::begin(_population_costs), std::end(_population_costs));
      int best = std::distance(std::begin(_population_costs), result);
      return _vector_population.at(_current_vec)[param] +
             F_VAL * (_vector_population.at(best)[param] - _vector_population.at(_current_vec)[param]) +
             F_VAL * (_vector_population.at(mutation_vec_indizes[0])[param] -
                      _vector_population.at(mutation_vec_indizes[1])[param]);
#endif
    }
};

} // namespace "atf"

#endif /* differential_evolution_h */