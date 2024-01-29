#define ENABLE_OPENCL_COST_FUNCTION
#include <cppatf.hpp>

class tunable_gaussian {
public:
    explicit tunable_gaussian( atf::configuration &config ) : _CACHE_BLOCK_SIZE(config["CACHE_BLOCK_SIZE"].value()) {}

    // computes gaussian in-place in `data` for simplicity
    void operator()(std::vector<float> &data, size_t N) {
        for (size_t cache_block_offset = 0; cache_block_offset < N; cache_block_offset += _CACHE_BLOCK_SIZE)
            for (size_t i = cache_block_offset; i < cache_block_offset + _CACHE_BLOCK_SIZE; ++i)
                for (size_t j = 0; j < N; ++j)
                    data[ (i+1) * N + (j+1) ] = ( data[(i  ) * N + (j)] + data[(i  ) * N + (j+1)] + data[(i  ) * N + (j+2)] +
                                                  data[(i+1) * N + (j)] + data[(i+1) * N + (j+1)] + data[(i+1) * N + (j+2)] +
                                                  data[(i+2) * N + (j)] + data[(i+2) * N + (j+1)] + data[(i+2) * N + (j+2)] ) / 9.0f;
    };
private:
    size_t _CACHE_BLOCK_SIZE;
};

int main( int argc, char* argv[] )
{
  // input size
  int N = 1000;

  // Step 1: Generate the Search Space
  auto CACHE_BLOCK_SIZE = atf::tuning_parameter( "CACHE_BLOCK_SIZE"           ,
                                                 atf::interval<size_t>( 1,N ) ,
                                                 atf::divides( N )            );

  // Steps 2 & 3: Program-Guided Search Space Exploration
  auto tuner = atf::tuner().tuning_parameters( CACHE_BLOCK_SIZE )
                           .search_technique( atf::auc_bandit() );

  std::vector<float> data( (N+3) * (N+3) ); for (int i = 0; i < data.size(); ++i) data[i] = static_cast<float>((i % 10) + 1);

  auto gaussian_cf = atf::cpp<tunable_gaussian>( data, N );

  for (int iter = 0; iter < 8; ++iter)
  {
    tuner.make_step( gaussian_cf );
  }
}
