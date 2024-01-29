#include <cppatf.hpp>

#define N 100

class mmm_block {
public:
  explicit mmm_block(atf::configuration &config) : BLOCK_SIZE(config["BLOCK_SIZE"].value()) {}

  void operator()(int (&a)[N][N], int (&b)[N][N], int (&c)[N][N]) const
  {
    int n = BLOCK_SIZE * (N/BLOCK_SIZE);
    int sum = 0;
    for(int k1=0;k1<n;k1+=BLOCK_SIZE)
    {
      for(int j1=0;j1<n;j1+=BLOCK_SIZE)
      {
        for(int k1=0;k1<n;k1+=BLOCK_SIZE)
        {
          for(int i=0;i<n;i++)
          {
            for(int j=j1;j<j1+BLOCK_SIZE;j++)
            {
              sum = c[i][j];
              for(int k=k1;k<k1+BLOCK_SIZE;k++)
              {
                sum += a[i][k] * b[k][j];
              }
              c[i][j] = sum;
    }}}}}
  }

private:
  int BLOCK_SIZE;
};

int main()
{
  // Step 1: Generate the Search Space
  auto BLOCK_SIZE = atf::tuning_parameter( "BLOCK_SIZE", atf::interval<int>( 1,10 ) );

  // Step 2: Implement a Cost Function
  int a[N][N];
  int b[N][N];
  int c[N][N];
  auto cxx_cf = atf::cxx::cost_function<mmm_block>( a, b, c );

  // Step 3: Explore the Search Space
  auto tuning_result = atf::tuner().tuning_parameters( BLOCK_SIZE )
                                   .search_technique( atf::exhaustive() )
                                   .tune( cxx_cf, atf::duration<std::chrono::seconds>( 30 ) );
}