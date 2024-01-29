#ifndef ATF_SEARCH_TECHNIQUE_HPP
#define ATF_SEARCH_TECHNIQUE_HPP

#include <set>

namespace atf {

using coordinates = std::vector<double>;
using cost_t = double;

/**
 * Searches over multi-dimensional coordinate space (0,1]^D.
 */
class search_technique {
  public:
    /**
     * Initializes the search technique.
     *
     * @param dimensionality "D" of the coordinate space
     */
    virtual void initialize(size_t dimensionality) = 0;

    /**
     * Finalizes the search technique.
     */
    virtual void finalize() = 0;

    /**
     * Returns the next coordinates in (0,1]^D for which the costs are requested.
     *
     * Function `get_next_coordinates()` is called by ATF before each call to `report_costs(...)`.
     *
     * @return coordinates in (0,1]^D
     */
    virtual std::set<coordinates> get_next_coordinates() = 0;

    /**
     * Processes costs for coordinates requested via function `get_next_coordinates()`.
     *
     * Function `report_costs(...)` is called by ATF after each call to `get_next_coordinates()`.
     *
     * @param costs coordinates mapped to their costs
     */
    virtual void report_costs(const std::map<coordinates, cost_t>& costs) = 0;
};

}

#endif //ATF_SEARCH_TECHNIQUE_HPP
