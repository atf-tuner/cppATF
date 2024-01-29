
#ifndef cpp_cf_h
#define cpp_cf_h

#include <chrono>

namespace atf
{

template<typename T, typename... Ts>
auto cpp( Ts&&... args ) {
    return [&](atf::configuration &config) -> cost_t {
        auto tunable = T(config);

        auto start = std::chrono::steady_clock::now();

        tunable( args... );

        auto end = std::chrono::steady_clock::now();
        auto cost = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        return static_cast<cost_t>(cost);
    };
}

} // namespace atf

#endif /* cpp_cf_h */
