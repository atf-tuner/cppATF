#define ENABLE_OPENCL_COST_FUNCTION
#include <cppatf.hpp>

int main( int argc, char* argv[] )
{
    // kernel code from path
    auto sgemm_kernel_as_string = atf::path("../cltune_gemm.cl");

    // input size
    int M = 8;
    int N = 8;
    int K = 8;

    // Query Device-Specific OpenCL Limits from ATF
    auto max_wi_sizes = atf::opencl::max_work_item_sizes();
    auto max_wg_size = atf::opencl::max_work_group_size();
    auto local_mem_size = atf::opencl::local_mem_size();

    // Step 1: Generate the Search Space
    auto MWG = atf::tuning_parameter("MWG", atf::interval<int>(1, M),
                                     atf::divides(M));
    auto NWG = atf::tuning_parameter("NWG", atf::interval<int>(1, N),
                                     atf::divides(N));
    auto KWG = atf::tuning_parameter("KWG", atf::interval<int>(1, K),
                                     atf::divides(K));

    auto MDIMC = atf::tuning_parameter("MDIMC", atf::interval<int>(1, M),
                                       atf::divides(MWG)
                                       && atf::less_than_or_eq(max_wi_sizes[0]));
    auto NDIMC = atf::tuning_parameter("NDIMC", atf::interval<int>(1, N),
                                       atf::divides(NWG)
                                       && atf::less_than_or_eq(max_wi_sizes[1])
                                       && [&](auto NDIMC) { return MDIMC * NDIMC <= max_wg_size; });
    auto MDIMA = atf::tuning_parameter("MDIMA", atf::interval<int>(1, M),
                                       atf::divides(MWG)
                                       && [&](auto MDIMA) { return (MDIMC * NDIMC) % MDIMA == 0; }
                                       && [&](auto MDIMA) { return KWG % ((MDIMC * NDIMC) / MDIMA) == 0; });
    auto NDIMB = atf::tuning_parameter("NDIMB", atf::interval<int>(1, N),
                                       atf::divides(NWG)
                                       && [&](auto NDIMB) { return (MDIMC * NDIMC) % NDIMB == 0; }
                                       && [&](auto NDIMB) { return KWG % ((MDIMC * NDIMC) / NDIMB) == 0; });

    auto KWI = atf::tuning_parameter("KWI", atf::interval<int>(1, K),
                                     atf::divides(KWG));

    auto VWM = atf::tuning_parameter("VWM", {1, 2, 4, 8},
                                     atf::divides(MWG / MDIMC)
                                     && atf::divides(MWG / MDIMA));
    auto VWN = atf::tuning_parameter("VWN", {1, 2, 4, 8},
                                     atf::divides(NWG / NDIMC)
                                     && atf::divides(NWG / NDIMB));

    auto STRM = atf::tuning_parameter("STRM", {0, 1});
    auto STRN = atf::tuning_parameter("STRN", {0, 1});

    auto SA = atf::tuning_parameter("SA", {0, 1});
    auto SB = atf::tuning_parameter("SB", {0, 1},
                                    [&](auto SB) {
                                        return ((SA * KWG * MWG / VWM) + (SB * KWG * NWG / VWN)) * sizeof(float) <=
                                               local_mem_size;
                                    }); // restriction of local memory

    // Step 2: Implement a Cost Function
    auto sgemm_kernel = atf::opencl::kernel< atf::scalar<int>   ,  // M
                                             atf::scalar<int>   ,  // N
                                             atf::scalar<int>   ,  // K
                                             atf::buffer<float> ,  // a
                                             atf::buffer<float> ,  // b
                                             atf::buffer<float> >  // c
                                           ( sgemm_kernel_as_string, "gemm_fast", " -DPRECISION=32" );  // kernel's code & name

    auto cf_sgemm = atf::opencl::cost_function( sgemm_kernel ).platform_id( 0 )  // OpenCL platform id
                                                              .device_id( 0 )    // OpenCL device id
                                                              .inputs( atf::scalar<int>( M )     ,  // M
                                                                       atf::scalar<int>( N )     ,  // N
                                                                       atf::scalar<int>( K )     ,  // K
                                                                       atf::buffer<float>( M*K ) ,  // a
                                                                       atf::buffer<float>( N*K ) ,  // b
                                                                       atf::buffer<float>( M*N ) )  // c
                                                              .global_size( ((1 + ((M - 1) / MWG))*MWG * MDIMC) / MWG ,  // OpenCL global size
                                                                            ((1 + ((N - 1) / NWG))*NWG * NDIMC) / NWG )
                                                              .local_size(  MDIMC                                     ,  // OpenCL local size
                                                                            NDIMC                                     );

    // Step 3: Explore the Search Space
    auto tuning_result = atf::tuner().tuning_parameters( MWG,NWG,KWG,
                                                         MDIMC,NDIMC,MDIMA,NDIMB,
                                                         KWI,
                                                         VWM,VWN,
                                                         STRM,STRN,
                                                         SA,SB )
                                     .search_technique( atf::auc_bandit() )
                                     .tune( cf_sgemm, atf::evaluations(50) );
}
