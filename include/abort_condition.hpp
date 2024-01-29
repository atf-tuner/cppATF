#ifndef ATF_ABORT_CONDITION_HPP
#define ATF_ABORT_CONDITION_HPP

#include "tuning_status.hpp"

namespace atf {

class abort_condition {
  public:

    /**
     * Determines whether a tuning run should be stopped based on its tuning status.
     *
     * @param status The current status of the tuning run (best found configuration so far, tuning time, ...)
     * @return true, if the tuning should stop, false otherwise
     */
    virtual bool stop(const tuning_status& status) = 0;
};

}

#endif //ATF_ABORT_CONDITION_HPP
