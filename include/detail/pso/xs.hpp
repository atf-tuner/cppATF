#ifndef pso_xs_h
#define pso_xs_h

#include <random>

namespace atf {
  namespace detail {
    namespace pso {
      namespace xs {
        /**
         * \brief Default crossover functor using constriction coefficients.
         *
         * The crossover function is implemented as explained as constriction coefficients in the paper
         * `Particle swarm optimization - An overview` by Riccardo Poli, James Kennedy and Tim Blackwell.
         */
        struct def {
          using distribution_type = std::uniform_real_distribution<>;

          static constexpr float phi_one = 2.05;
          static constexpr float phi_two = 2.05;
          static constexpr float phi = phi_one + phi_two;

          /**
           * \brief Implements crossover using constriction coefficients.
           *
           * Will also set the current particle's velocity as it is used by this implementation as well.
           *
           * \param current     current particle to move
           * \param best        best particle of the swarm
           * \param dist        error function
           * \return            the current particle's new position
           */
          coordinates operator()(particle& current, const particle& best, distribution_type& dist)
          {
            std::mt19937 rnd;
            rnd.seed(std::random_device()());

            const double X {2.0 / (4.10 - 2.0 + std::sqrt(std::pow(4.10, 2.0) - 4.0 * 4.10))};
            coordinates new_velocity {
               (current.velocity() + ((current.best_position() - current.position()) * dist(rnd)) +
               ((best.position() - current.position()) * dist(rnd)))
               * X
            };

            current.set_velocity(new_velocity);

            return current.position() + new_velocity;
          }
        };

        /**
         * \brief Crossover functor implementing crossover from OpenTuner.
         */
        struct open_tuner {
          using distribution_type = std::uniform_real_distribution<>;

          static constexpr float c = 0.5;
          static constexpr float phi_one = 1.0;

          /**
           * \brief Implements crossover as found in the OpenTuner.
           *
           * \param current     the current particle to move
           * \param best        the best particle of the swarm
           * \param dist        error function
           * \return            the current particle's new position
           */
          coordinates operator()(particle& current, const particle& best, distribution_type& dist)
          {
            /*
             * vmin, vmax = self.legal_range(cfg)
             * k = vmax - vmin
             * # calculate the new velocity
             * v = velocity * c + (self.get_value(cfg1) - self.get_value(
             *     cfg)) * c1 * random.random() + (self.get_value(
             *     cfg2) - self.get_value(cfg)) * c2 * random.random()
             * # Map velocity to continuous space with sigmoid
             * s = k / (1 + numpy.exp(-v)) + vmin
             * # Add Gaussian noise
             * p = random.gauss(s, sigma * k)
             * # Discretize and bound
             * p = int(min(vmax, max(round(p), vmin)))
             * self.set_value(cfg, p)
             * return v
             */

            std::mt19937 rnd;
            rnd.seed(std::random_device()());

            coordinates new_velocity {
              current.velocity() * c + (current.best_position() - current.position())
              * c * dist(rnd) + (best.position() - current.position()) * c * dist(rnd)
            };

            // Boundaries
            for (auto& v : new_velocity) {
              v = std::min(1.0, std::max(v, -1.0));
            }

            current.set_velocity(new_velocity);

            return (current.position() + new_velocity);
          }

        };

        /**
         * \brief Crossover functor implementing crossover from CLTune.
         */
        struct cl_tune {
          using distribution_type = std::uniform_real_distribution<>;

          static constexpr float phi_one = 1.0;

          /**
           * \brief Implements crossover as found in the CLTune.
           *
           * \param current     the current particle to move
           * \param best        the best particle of the swarm
           * \param dist        error function
           * \return            the current particle's new position
           */
          coordinates operator()(particle& current, const particle& best, distribution_type& dist)
          {
            const double influence_global = 0.333333;
            const double influence_local = 0.333333;
            const double influence_random = 0.333333;
            /*
             * CLTune:
             *
             * // Move towards best known globally (swarm)
             * if (probability_distribution_(generator_) <= influence_global_) {
             *   next_configuration[i].value = global_best_config_[i].value;
             * }
             * // Move towards best known locally (particle)
             * else if (probability_distribution_(generator_) <= influence_local_) {
             *   next_configuration[i].value = local_best_configs_[particle_index_][i].value;
             * }
             * // Move in a random direction
             * else if (probability_distribution_(generator_) <= influence_random_) {
             *   std::uniform_int_distribution<size_t> distribution(0, parameters_[i].values.size());
             *   next_configuration[i].value = parameters_[i].values[distribution(generator_)];
             * }
             * // Else: stay at current location
             */
            std::mt19937 rnd;
            rnd.seed(std::random_device()());

            coordinates new_pos(current.position().size());
            std::size_t i;
            std::generate(new_pos.begin(), new_pos.end(), [&]()
            {
              for (i = 0; i < current.position().size(); ++i) {
                if (dist(rnd) <= influence_global) {
                  return best.position()[i];
                } else if (dist(rnd) <= influence_local) {
                  return current.best_position()[i];
                } else if (dist(rnd) <= influence_random) {
                  return dist(rnd);
                }
              }
            });

            return new_pos;
          }
        };

      } /* namespace xs */
    } /* namespace pso */
  } /* namespace detail */
} /* namespace atf */

#endif //pso_xs_h
