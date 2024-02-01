#include <cppatf.hpp>

int main()
{
  // Step 1: Generate the Search Space
  auto BLOCK_SIZE = atf::tuning_parameter( "BLOCK_SIZE", atf::interval<int>( 1,10 ) );

  // Step 2: Implement a Cost Function
  auto run_command     = "./tmp.bin";
  auto compile_command = "g++ ../mmm_block.cpp -DBLOCK_SIZE=$BLOCK_SIZE -o ./tmp.bin";

  auto cf_matmul = atf::generic::cost_function( run_command ).compile_command( compile_command );

  // Step 3: Explore the Search Space
  auto tuning_result = atf::tuner().tuning_parameters( BLOCK_SIZE )
                                   .search_technique( atf::exhaustive() )
                                   .tune( cf_matmul );
}