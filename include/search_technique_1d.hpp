#ifndef ATF_SEARCH_TECHNIQUE_1D_HPP
#define ATF_SEARCH_TECHNIQUE_1D_HPP

#include <set>
#include <map>
#include "big_int.hpp"

namespace atf {

using index = atf::big_int;
using cost_t = double;

/**
 * Searches over one-dimensional index space { 0 , ... , |SP|-1 }, where |SP| is the search space size.
 */
class search_technique_1d {
  public:
    /**
     * Initializes the search technique.
     *
     * @param search_space_size the total number of configurations in the search space
     */
    virtual void initialize(atf::big_int search_space_size) = 0;

    /**
     * Finalizes the search technique.
     */
    virtual void finalize() = 0;

    /**
     * Returns the next indices in { 0 , ... , |SP|-1 } for which the costs are requested.
     *
     * Function `get_next_indices()` is called by ATF before each call to `report_costs(...)`.
     *
     * @return indices in { 0 , ... , |SP|-1 }
     */
    virtual std::set<index> get_next_indices() = 0;

    /**
     * Processes costs for indices requested via function `get_next_indices()`.
     *
     * Function `report_costs(...)` is called by ATF after each call to `get_next_indices()`.
     *
     * @param costs indices mapped to their costs
     */
    virtual void report_costs(const std::map<index, cost_t>& costs) = 0;
};

}

#endif //ATF_SEARCH_TECHNIQUE_1D_HPP
