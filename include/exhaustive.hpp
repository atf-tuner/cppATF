
#ifndef exhaustive_h
#define exhaustive_h

#include "search_technique_1d.hpp"

namespace atf
{

class exhaustive : public search_technique_1d
{
  public:
    big_int _search_space_size;

    void initialize(big_int search_space_size) override
    {
      _search_space_size = search_space_size;
    }
  
  
    std::set<index> get_next_indices() override
    {
      static big_int pos = 0;
      
      if( pos == _search_space_size )
        pos = 0;
      
      return { pos++ };
    }
  
    
    void report_costs(const std::map<index, cost_t>& costs) override
    {}
  
  
    void finalize() override
    {}
};

} // namespace "atf"



#endif /* exhaustive_h */
