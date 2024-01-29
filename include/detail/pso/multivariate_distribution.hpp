#ifndef pso_multivariate_normal_distribution_h
#define pso_multivariate_normal_distribution_h

#include <random>

namespace atf {
  namespace detail {
    namespace pso {

      /**
       * \brief Provides a multidimensional probability distribution.
       *
       * It is meant to be used a convenience wrapper to use for randomised positions in an
       * n-th dimensional coordinate space.
       *
       * \tparam PointType  type of point to use
       * \tparam DistType   type of underlying distribution
       */
      template <typename PointType = ::atf::coordinates, typename DistType = ::std::normal_distribution<>>
      class multivariate_distribution {
      public:
        using point_type = PointType;
        using dist_type = DistType;

        /**
         * \brief Constructs a new multivariate distribution around a point with given covariance matrix.
         *
         * \param mean      central position, the mean position
         * \param cov_mat   the convariance matrix to use
         */
        multivariate_distribution(const point_type& mean, const std::vector<point_type>& cov_mat)
          : _dists(mean.size())
        {
          std::generate(_dists.begin(), _dists.end(), [&, i = 0]() mutable
          {
            int tmp_i = i++;
            return dist_type {mean[tmp_i], cov_mat[tmp_i][tmp_i]};
          });
        }

        /**
         * \brief Constructs a new multivariate distribution around a point with given covariances.
         *
         * \param mean          central position, the mean position
         * \param cov_mat_diag  the covariances to use
         */
        multivariate_distribution(const point_type& mean, const point_type& cov_mat_diag)
          : _dists(mean.size())
        {
          std::generate(_dists.begin(), _dists.end(), [&, i = 0]() mutable
          {
            int tmp_i = i++;
            return dist_type {mean[tmp_i], cov_mat_diag[tmp_i]};
          });
        }

        /**
         * \brief Maps a from the the generator created value into the distributions domain.
         *
         * \tparam Generator    type of the value generator
         * \param g             the value generator
         * \return              a new randomised vector
         */
        template <typename Generator>
        point_type operator()(Generator& g)
        {
          point_type result(_dists.size());
          
          std::generate(result.begin(), result.end(), [&, i = 0]() mutable
          {
            return _dists[i++](g);
          });

          return result;
        }

        /**
         * \brief Resets each dimension's distribution.
         */
        void reset()
        {
          for (auto& dist : _dists) {
            dist.reset();
          }
        }

      private:
        /** Vector containing one distribution for each dimension of the domain space */
        std::vector<dist_type> _dists;
      };

    } /* namespace pso */
  } /* namespace detail */
} /* namespace atf */
#endif
