#define ENABLE_OPENCL_COST_FUNCTION
#include <cppatf.hpp>

int main( int argc, char* argv[] )
{
    // kernel code as string
    auto sum_kernel_as_string = R"(
void atomic_add_f(volatile global float* addr, const float val) {
    private float old, sum;
    do {
        old = *addr;
        sum = old+val;
    } while(atomic_cmpxchg((volatile global int*)addr, as_int(old), as_int(sum))!=as_int(old));
}

__kernel void sum( const int N, const __global float* x, __global float* y )
{
    for( int w = 0 ; w < WPT ; ++w )
    {
        const int index = w * get_global_size(0) + get_global_id(0);
        atomic_add_f( y , x[ index ] );
    }
})";
    
    // input size
    int N = 1000;

    // compute gold
    std::default_random_engine dre{std::random_device()()};
    std::uniform_real_distribution<float> urd(0.0f, 1.0f);
    std::vector<float> x( N ); for (int i = 0; i < N; ++i) x[i] = urd(dre);
    std::vector<float> y( 1 ); y[0] = 0.0f;
    std::vector<float> y_gold( 1 );
    for (int i = 0; i < N; ++i)
      y_gold[0] += x[i];

    // Step 1: Generate the Search Space
    auto WPT = atf::tuning_parameter( "WPT"                        ,
                                      atf::interval<size_t>( 1,N ) ,
                                      atf::divides( N )            );
    
    auto LS  = atf::tuning_parameter( "LS"                         ,
                                      atf::interval<size_t>( 1,N ) ,
                                      atf::divides( N/WPT )        );

    // Step 2: Implement a Cost Function
    auto sum_kernel = atf::opencl::kernel< atf::scalar<int>   ,  // N
                                           atf::buffer<float> ,  // x
                                           atf::buffer<float> >  // y
                                         ( atf::source(sum_kernel_as_string), "sum" );  // kernel's code & name

    auto cf_sum = atf::opencl::cost_function( sum_kernel ).platform_id( 0 )                   // OpenCL platform id
                                                          .device_id( 0 )                     // OpenCL device id
                                                          .inputs( atf::scalar<int>( N )   ,  // N
                                                                   atf::buffer<float>( x ) ,  // x
                                                                   atf::buffer<float>( y ) )  // y
                                                          .global_size( N/WPT )               // OpenCL global size
                                                          .local_size( LS )                   // OpenCL local size
                                                          .check_result<2>( y_gold                           ,  // check result for buffer with index 2 (the "y" buffer) against pre-calculated gold vector
                                                                            atf::absolute_difference(0.001f) )  // -> due to floating point inaccuracies, consider the result correct if the absolute difference does not exceed 0.001f (default: 0.000f)
                                                          .check_result<2>( [](int N, const std::vector<float> &x, const std::vector<float> &y)     // check result for buffer with index 2 (the "y" buffer) against callable that produces a gold vector
                                                                            {
                                                                              std::vector<float> y_gold(1);
                                                                              y_gold[0] = 0.0f;
                                                                              for (int i = 0; i < N; ++i) {
                                                                                y_gold[0] += x[i];
                                                                              }
                                                                              return y_gold;
                                                                            }                                                                   ,
                                                                            atf::absolute_difference(0.001f)                                    );  // -> due to floating point inaccuracies, consider the result correct if the absolute difference does not exceed 0.001 (default: 0.000f)

    // Step 3: Explore the Search Space
    auto tuning_result = atf::tuner().tuning_parameters( WPT, LS )
                                     .search_technique( atf::auc_bandit() )
                                     .tune( cf_sum, atf::evaluations(50) );
}
