#ifndef random_search_hpp
#define random_search_hpp

#include <random>
#include <chrono>
#include <fstream>

#include "search_technique_1d.hpp"

namespace atf
{

class random_search : public search_technique_1d
{
  public:
    void initialize( big_int search_space_size ) override
    {
      _search_space_size = search_space_size;
    }


    std::set<index> get_next_indices() override
    {
      return { big_int(0, _search_space_size) };
    }


    void report_costs( const std::map<index, cost_t>& costs ) override
    {}


    void finalize() override
    {}

  private:
    big_int _search_space_size;
};

} // namespace "atf"



#endif /* random_search_hpp */