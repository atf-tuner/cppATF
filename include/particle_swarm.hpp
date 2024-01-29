#ifndef particle_swarm_h
#define particle_swarm_h

#include <random>
#include <vector>

#include "search_technique.hpp"
#include "detail/pso/multivariate_distribution.hpp"
#include "detail/pso/particle.hpp"
#include "detail/pso/swarm.hpp"
#include "detail/pso/xs.hpp"


namespace atf {

/**
 * \tparam N      number of swarms
 * \tparam M      number of particles per swarm
 * \tparam XFunc  type of crossover functor
 */
template<std::size_t N, std::size_t M, typename XFunc>
class particle_swarm_class : public search_technique {
  public:

    using x_func = XFunc;
    using distribution_type = typename XFunc::distribution_type;

    particle_swarm_class()
        : _dist {0.0, XFunc::phi_one}
    {}

    void initialize(size_t dimensionality) override
    {
      _dimensionality = dimensionality;

      _swarms.reserve(N);
      for (std::size_t i = 0; i < N; ++i) {
        _swarms.emplace_back(_dimensionality);
      }
    }

    std::set<coordinates> get_next_coordinates() override
    {
      ++_pos;
      if (_pos == M * N) {
        _pos = 0;

        for (auto &sw : _swarms) {
          sw.move(_dist, x_func{});
        }
      }

      /* Get current particle */
      detail::pso::particle& p = _swarms[_pos % N][_pos % M];
      /* Check if current particle has valid position. If not, use fmod to map its position to the coordinate_space. */
      if (!valid_coordinates(p.position())) {
        p.set_position(clamp_coordinates_mod(p.position()));
      }

      return { p.position() };
    }

    void report_costs( const std::map<coordinates, cost_t>& costs ) override
    {
      cost_t cost = costs.begin()->second;
      _swarms[_pos % N].report_fitness(cost, _pos % M);
    }

    void finalize() override
    {

    }

  private:
    /** dimensionality search space */
    size_t _dimensionality{0};
    /** vector of swarms to use */
    std::vector<detail::pso::swarm<M>> _swarms;
    /** probability distribution to use for crossover functor */
    distribution_type _dist;
    /** index of current particle */
    std::size_t _pos {0};
};

/**
 * \brief Factory function constructing a new instance of the particle swarm search technique.
 *
 * \tparam N      number of swarms to use
 * \tparam M      number of particles per swarm
 * \tparam XFunc  type of crossover functor
 * \return        new instance of the particle swarm search technique
 */
template<std::size_t N = 1, std::size_t M = 30, typename XFunc = detail::pso::xs::def>
auto particle_swarm()
{
  return particle_swarm_class<N, M, XFunc>{};
}

} /* namespace atf */

#endif
