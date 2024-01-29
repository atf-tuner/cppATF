#ifndef ATF_TUNING_STATUS_HPP
#define ATF_TUNING_STATUS_HPP

#include <tuple>
#include "tp_value.hpp"

namespace atf {

using cost_t = double;

class tuning_status {
public:
    auto best_configuration() const {
        return std::get<1>(_history.back());
    }
    auto min_cost() const {
        return std::get<2>(_history.back());
    }
    auto number_of_evaluated_configs() const {
        return _number_of_evaluated_configs;
    }
    auto number_of_invalid_configs() const {
        return _number_of_invalid_configs;
    }
    auto number_of_valid_configs() const {
        return _number_of_evaluated_configs - _number_of_invalid_configs;
    }
    auto evaluations_required_to_find_best_found_result() const {
        return _evaluations_required_to_find_best_found_result;
    }
    auto valid_evaluations_required_to_find_best_found_result() const {
        return _valid_evaluations_required_to_find_best_found_result;
    }
    auto history() const {
        return _history;
    }
    auto tuning_start_time() const {
        return std::get<0>( _history.front() );
    }

private:
    friend class exploration_engine;
    friend class tuner;

    size_t                                      _number_of_evaluated_configs;
    size_t                                      _number_of_invalid_configs;
    size_t                                      _evaluations_required_to_find_best_found_result;
    size_t                                      _valid_evaluations_required_to_find_best_found_result;
    using                                        history_entry = std::tuple< std::chrono::steady_clock::time_point, configuration, cost_t >; // entry: actual tuning runtime, configuration, configuration's cost
    std::vector<history_entry>                  _history; // history of best results
};

}

#endif //ATF_TUNING_STATUS_HPP
