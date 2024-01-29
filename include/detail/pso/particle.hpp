#ifndef pso_particle_h
#define pso_particle_h

namespace atf {
  namespace detail {
    namespace pso {

      /**
       * \brief Represents a single particle.
       *
       * This class respresents a single particle of the particle swarm optimisation.
       * Each particle has a position in the coordinate space, a velocity, a best position and
       * a fitness value determined by the configuration that is respresented by its current
       * position.
       */
      class particle {
      public:
        /**
         * \brief Constructs a new particle on a specific position.
         *
         * Copy-constructs particle's position from provided point.
         *
         * @param start_position    position of the new particle
         */
        explicit particle(const coordinates& start_position)
          : _pos {start_position}
          , _best_pos {start_position}
          , _velocity {std::vector<double>(_pos.size(), 0.0)}
        {
        }

        /**
         * \brief Constructs a new particle on a specific position.
         *
         * Move-constructs particle's position from provided coordinates.
         *
         * @param start_position    position of the new particle
         */
        explicit particle(coordinates&& start_position)
          : _pos {std::move(start_position)}
          , _best_pos {_pos}
          , _velocity {std::vector<double>(_pos.size(), 0.0)}
        {
        }

        /**
         * \brief Returns the particle's current position.
         *
         * \return the particle's current position
         */
        const coordinates& position() const
        {
          return _pos;
        }

        /**
         * \brief Returns the particle's current velocity.
         *
         * \return the particle's current velocity
         */
        const coordinates& velocity() const
        {
          return _velocity;
        }

        /**
         * \brief Returns the particle's best position.
         *
         * \return the particle's best position
         */
        const coordinates& best_position() const
        {
          return _best_pos;
        }

        /**
         * \brief Sets the particle's current velocity.
         *
         * Copy-assigns the new velocity to the current one.
         *
         * This is mainly used by the crossover functors to adjust
         * the particles velocity and should normally not used
         * in any other case.
         *
         * \velocity the new velocity
         */
        void set_velocity(const coordinates& velocity)
        {
          _velocity = velocity;
        }

        /**
         * \brief Sets the particle's current velocity.
         *
         * Move-assigns the new velocity to the current one.
         *
         * This is mainly used by the crossover functors to adjust
         * the particles velocity and should normally not be used
         * in any other case.
         *
         * \velocity the new velocity
         */
        void set_velocity(coordinates&& velocity)
        {
          _velocity = velocity;
        }

        /**
         * \brief Sets the particle's current position.
         *
         * Copy-assigns the new velocity to the current one.
         *
         * This is mainly used by the swarm class to reassign
         * the particle's position if it is currently not
         * in the coordinate space and should normally not be used
         * in any other case
         *
         * \position the new position
         */
        void set_position(const coordinates& position)
        {
          _pos = position;
        }

        /**
         * \brief Sets the particle's current position.
         *
         * Move-assigns the new velocity to the current one.
         *
         * This is mainly used by the swarm class to reassign
         * the particle's position if it is currently not
         * in the coordinate space and should normally not be used
         * in any other case
         *
         * \position the new position
         */
        void set_position(coordinates&& position)
        {
          _pos = position;
        }

        /**
         * \brief Returns the particle's current fitness value.
         * \return the particle's current fitness value
         */
        double fitness() const
        {
          return _best_fitness;
        }

        /**
         * \brief Reports new fitness to the particle.
         *
         * If the new reported fitness is better than the current best
         * fitness, the old best fitness will be overridden with the new
         * value and the current position marked as best position.
         *
         * \param fit   the particle's new determined fitness
         */
        void report_fitness(double fit)
        {
          if (_best_fitness > fit) {
            _best_fitness = fit;
            _best_pos = _pos;
          }
        }

        /**
         * \brief Moves the particle according to specific crossover function.
         *
         * The provided functor decides, how the swarm's best particle, the current
         * particle's (best) position and velocity and the provided error function is used to
         * determine a new position for the current particle.
         *
         * This way it is possible to support multiple crossover functions by outsourcing movement
         * logic from the particle itself to an external functor, which can also be user-provided.
         *
         * \tparam XFunc    type of the crossover functor
         *
         * \param best      swarm's best particle
         * \param dist      error function
         * \param x_func    instance of the crossover functor
         */
        template <typename XFunc>
        void move(const particle& best, typename XFunc::distribution_type& dist, XFunc&& x_func)
        {
          _pos = x_func(*this, best, dist);
        }

      private:
        /** Current position */
        coordinates _pos;
        /** Current velocity */
        coordinates _velocity;
        /** Last best position */
        coordinates _best_pos;
        /** Best found fitness value */
        double _best_fitness {std::numeric_limits<double>::max()};
      };

    } /* namespace pso */
  } /* namespace detail */
} /* namespace atf */

#endif