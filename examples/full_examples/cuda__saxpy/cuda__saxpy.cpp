#define ENABLE_CUDA_COST_FUNCTION
#include <cppatf.hpp>

int main( int argc, char* argv[] )
{
    // kernel code as string
    auto saxpy_kernel_as_string = R"(
extern "C" __global__ void saxpy( const int N, const float a, const float* x, float* y )
{
    for( int w = 0 ; w < WPT ; ++w )
    {
        const int index = w * gridDim.x * blockDim.x + blockIdx.x * blockDim.x + threadIdx.x;
        y[ index ] += a * x[ index ];
    }
})";
    
    // input size
    int N = 1000;

    // Step 1: Generate the Search Space
    auto WPT = atf::tuning_parameter( "WPT"                        ,
                                      atf::interval<size_t>( 1,N ) ,
                                      atf::divides( N )            );
    
    auto LS  = atf::tuning_parameter( "LS"                         ,
                                      atf::interval<size_t>( 1,N ) ,
                                      atf::divides( N/WPT )        );

    // Step 2: Implement a Cost Function
    auto saxpy_kernel = atf::cuda::kernel< atf::scalar<int>   ,  // N
                                           atf::scalar<float> ,  // a
                                           atf::buffer<float> ,  // x
                                           atf::buffer<float> >  // y
                                         ( atf::source(saxpy_kernel_as_string), "saxpy" );  // kernel's code & name

    auto cf_saxpy = atf::cuda::cost_function( saxpy_kernel ).device_id( 0 )                     // CUDA device id
                                                            .inputs( atf::scalar<int>( N )   ,  // N
                                                                     atf::scalar<float>()    ,  // a
                                                                     atf::buffer<float>( N ) ,  // x
                                                                     atf::buffer<float>( N ) )  // y
                                                            .grid_dim( N/WPT/LS )               // CUDA grid dim
                                                            .block_dim( LS );                   // CUDA block dim

    // Step 3: Explore the Search Space
    auto tuning_result = atf::tuner().tuning_parameters( WPT, LS )
                                     .search_technique( atf::auc_bandit() )
                                     .tune( cf_saxpy, atf::evaluations(50) );
}
