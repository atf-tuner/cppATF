#ifndef pso_swarm_h
#define pso_swarm_h

#include <random>
#include <vector>

#include "multivariate_distribution.hpp"
#include "particle.hpp"

namespace atf {
  namespace detail {
    namespace pso {

    /**
     * \brief Represents a swarm of particles.
     *
     * Internally, the swarm will make use of exactly `N` particles.
     *
     * \tparam N    the number of particles
     */
    template <std::size_t N>
    class swarm {
    public:
      using iterator = typename std::vector<particle>::iterator;
      using const_iterator = typename std::vector<particle>::const_iterator;

      /**
       * \brief Constructs a new swarm.
       *
       * \param num_dims    number of dimensions of the search space, the swarm shall operate on
       */
      explicit swarm(std::size_t num_dims)
        : _dist {coordinates(num_dims, -1.0), coordinates(num_dims, 1.0)}
      {
        _rng.seed(std::random_device()());

        /*
         * Reserve memory for the N particles and then use `emplace_back`
         * to make this as fast as possible for many particles without
         * unnecessary default initialization as before with std::array
         */
        _particles.reserve(N);
        for(std::size_t i = 0; i < N; ++i) {
          _particles.emplace_back(_dist(_rng));
        }
      }

      /**
       * \brief Provides access to the swarm's particles.
       *
       * Non-const-overload.
       *
       * \param pos     index of particle
       * \return        non-const reference to the particle
       */
      particle& operator[](std::size_t pos)
      {
        return _particles[pos];
      }

      /**
       * \brief Provides access to the swarm's particles.
       *
       * Const-overload.
       *
       * \param pos     index of particle
       * \return        const reference to the particle
       */
      const particle& operator[](std::size_t pos) const
      {
        return _particles[pos];
      }

      /**
       * \brief Moves the swarm.
       *
       * Therefore, moves each particle according to implementation
       * of crossover functor.
       *
       * Resets counter for invalid arguments in the swarm.
       *
       * \tparam XFunc      type of crossover functor
       *
       * \param dist        error function
       * \param x_func      instance of the crossover functor
       */
      template <typename XFunc>
      void move(typename XFunc::distribution_type& dist, XFunc&& x_func)
      {
        /* Reset invalid_configs */
        _invalid_configs = 0;

        const particle& best {_particles[find_best_particle()]};

        for (particle& p : _particles) {
          p.move(best, dist, std::forward<XFunc>(x_func));
        }
      }

      /**
       * \brief Provides iterator access to first particle.
       *
       * \return iterator to begin of the particle list
       */
      iterator begin()
      {
        return _particles.begin();
      }

      /**
       * \brief Provides iterator access to behind-last particle.
       *
       * \return iterator past the end of the particle list
       */
      iterator end()
      {
        return _particles.end();
      }

      /**
       * \brief Provides iterator access to first particle.
       *
       * Const-overload.
       *
       * \return const iterator to begin of the particle list
       */
      const_iterator begin() const
      {
        return _particles.begin();
      }

      /**
       * \brief Provides iterator access to behind-last particle.
       *
       * Const-overload.
       *
       * \return const iterator past the end of the particle list
       */
      const_iterator end() const
      {
        return _particles.end();
      }

      /**
       * \brief Reports fitness to a specific particle in the swarm.
       *
       * Fitness is reported to a particle through the swarm rather than
       * directly to the particle to be able to avoid swarm convergence on
       * an invalid configuration. For more details, see note.
       *
       * \note Before, the particles got their fitness directly reported by the implementation of the
       * ATF interface. However, because the ATF does possibly not make precise constraints on its
       * tuning parameter - this is for example the case in the GEMM-sample - it is possible to have a correct
       * position inside of the coordinate_space but still produce an invalid configuration. This seems to
       * happen when an invalid local memory size is chosen by the search-technique.
       * Because of this it is possible for a swarm of particles to converge on an invalid configuration.
       * As soon as all particles are in this state, movement away from this invalid configuration is impossible.
       * Workaround: If more than 50% of the particles of a swarm get a maximal cost value, the swarm will reset
       * the positions of ALL its particle.
       *
       * \param fitness         the particle's new determined fitness value
       * \param particle_index  the index of the particle whose fitness value is reported
       */
      void report_fitness(double fitness, std::size_t particle_index)
      {
        static constexpr std::size_t threshold = static_cast<std::size_t>(0.5 * N);

        _particles[particle_index].report_fitness(fitness);

        if (fitness == std::numeric_limits<double>::max()) {
          /* We encountered an error: increase counter */
          ++_invalid_configs;
        }

        if (_invalid_configs > threshold) {
          reset();
        }
      }

    private:

      /**
       * \brief Finds the best particle in the swarm.
       *
       * Particles are compared with regards to their current fitness values.
       *
       * \return index of particle with the best fitness value
       */
      std::size_t find_best_particle()
      {
        auto it = std::max_element(_particles.begin(), _particles.end(), [this](const auto& lhs, const auto& rhs)
        {
          return lhs.fitness() < rhs.fitness();
        });

        return static_cast<std::size_t>(std::distance(_particles.begin(), it));
      }

      /**
       * \brief Resets the whole swarm.
       *
       * This is done by reassigning each particle to a new random position.
       *
       * \note This is mainly used as a workaround for convergence on invalid configuration.
       */
      void reset()
      {
        /* Just reassign every particle ptr */
        std::generate(_particles.begin(), _particles.end(), [this]()
        {
          return particle {_dist(_rng)};
        });
      }

      /** multidimensional uniform distribution for random positions in the search space */
      multivariate_distribution<::atf::coordinates, ::std::uniform_real_distribution<>> _dist;
      /** random device for random positions */
      std::mt19937 _rng;
      /** vector containing all particles of the swarm */
      std::vector<particle> _particles;
      /** index of the last best particle found */
      std::size_t _best_idx {0};
      /** number of invalid configurations */
      std::size_t _invalid_configs {0};
    };

    } /* namespace pso */
  } /* namespace detail */
} /* namespace atf */

#endif
