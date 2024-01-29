#ifndef BIG_INT_INFINT_HPP
#define BIG_INT_INFINT_HPP

#include <random>
#define INFINT_USE_EXCEPTIONS
#include "../thirdparty/InfInt.hpp"
#undef INFINT_USE_EXCEPTIONS

namespace atf {

using big_int = big_int_interface<InfInt>;

// arithmetic operators
big_int operator+(const ::atf::big_int& lhs, const ::atf::big_int& rhs) {
  return {lhs._backend + rhs._backend};
}
big_int operator-(const ::atf::big_int& lhs, const ::atf::big_int& rhs) {
  return {lhs._backend - rhs._backend};
}
big_int operator*(const ::atf::big_int& lhs, const ::atf::big_int& rhs) {
  return {lhs._backend * rhs._backend};
}
big_int operator/(const ::atf::big_int& lhs, const ::atf::big_int& rhs) {
  try {
    return {lhs._backend / rhs._backend};
  } catch (const InfIntException& e) {
    throw big_int_exception(e.what());
  }
}
big_int operator%(const ::atf::big_int& lhs, const ::atf::big_int& rhs) {
  try {
    return {lhs._backend % rhs._backend};
  } catch (const InfIntException& e) {
    throw big_int_exception(e.what());
  }
}

// relational operators
bool operator==(const big_int& lhs, const big_int& rhs) {
  return lhs._backend == rhs._backend;
}
bool operator!=(const big_int& lhs, const big_int& rhs) {
  return lhs._backend != rhs._backend;
}
bool operator<(const big_int& lhs, const big_int& rhs) {
  return lhs._backend < rhs._backend;
}
bool operator<=(const big_int& lhs, const big_int& rhs) {
  return lhs._backend <= rhs._backend;
}
bool operator>(const big_int& lhs, const big_int& rhs) {
  return lhs._backend > rhs._backend;
}
bool operator>=(const big_int& lhs, const big_int& rhs) {
  return lhs._backend >= rhs._backend;
}

// io
std::ostream& operator<<(std::ostream &s, const big_int &n) {
  return s << n._backend;
}
std::istream& operator>>(std::istream &s, big_int &n) {
  return s >> n._backend;
}

// default constructor
template<>
big_int::big_int_interface() = default;

// initialization from other types
template<>
big_int::big_int_interface(int v) : _backend(v) {}
template<>
big_int::big_int_interface(long v) : _backend(v) {}
template<>
big_int::big_int_interface(long long v) : _backend(v) {}
template<>
big_int::big_int_interface(unsigned int v) : _backend(v) {}
template<>
big_int::big_int_interface(unsigned long v) : _backend(v) {}
template<>
big_int::big_int_interface(unsigned long long v) : _backend(v) {}
template<>
big_int::big_int_interface(const std::string& v) : _backend(v) {}
template<>
big_int::big_int_interface(const char* v) : _backend(v) {}
template<>
big_int::big_int_interface(const big_int &min, const big_int &max) {
    if (max <= min)
        throw std::runtime_error("min has to be smaller than max");
    // TODO: add checks to determine if double precision is sufficient to generate a random in the passed in interval
    InfInt interval_size = max._backend - min._backend;
    std::default_random_engine random_engine{std::random_device()()};
    std::uniform_real_distribution<double> dist{0.0, 1.0};
    std::stringstream dist_str;
    dist_str.precision(std::numeric_limits<double>::max_digits10);
    dist_str << std::fixed << dist(random_engine);
    InfInt scaled_random = InfInt(dist_str.str().substr(2));
    InfInt precision_factor = 1;
    for (int i = 0; i < scaled_random.numberOfDigits(); ++i)
        precision_factor *= 10;
    InfInt scaled_prod = interval_size * scaled_random;
    InfInt prod = scaled_prod / precision_factor;
    _backend = min._backend + prod;
}

// assignment of other types
template<>
big_int& big_int::operator=(int v) {
    _backend = v;
    return *this;
}
template<>
big_int& big_int::operator=(long  v) {
    _backend = v;
    return *this;
}
template<>
big_int& big_int::operator=(long long  v) {
    _backend = v;
    return *this;
}
template<>
big_int& big_int::operator=(unsigned int  v) {
    _backend = v;
    return *this;
}
template<>
big_int& big_int::operator=(unsigned long  v) {
    _backend = v;
    return *this;
}
template<>
big_int& big_int::operator=(unsigned long long  v) {
    _backend = v;
    return *this;
}
template<>
big_int& big_int::operator=(const std::string&  v) {
    _backend = v;
    return *this;
}

// cast to other types (throws big_int_exception when value is out of bounds)
template<>
big_int::operator int() const {
    try {
        return _backend.toInt();
    } catch (const InfIntException& e) {
        throw big_int_exception(e.what());
    }
}
template<>
big_int::operator long() const {
    try {
        return _backend.toLong();
    } catch (const InfIntException& e) {
        throw big_int_exception(e.what());
    }
}
template<>
big_int::operator long long() const {
    try {
        return _backend.toLongLong();
    } catch (const InfIntException& e) {
        throw big_int_exception(e.what());
    }
}
template<>
big_int::operator unsigned int() const {
    try {
        return _backend.toUnsignedInt();
    } catch (const InfIntException& e) {
        throw big_int_exception(e.what());
    }
}
template<>
big_int::operator unsigned long() const {
    try {
        return _backend.toUnsignedLong();
    } catch (const InfIntException& e) {
        throw big_int_exception(e.what());
    }
}
template<>
big_int::operator unsigned long long() const {
    try {
        return _backend.toUnsignedLongLong();
    } catch (const InfIntException& e) {
        throw big_int_exception(e.what());
    }
}
template<>
big_int::operator std::string() const {
    return _backend.toString();
}

// arithmetic operators
template<>
const big_int& big_int::operator++() {
    _backend.operator++();
    return *this;
}
template<>
const big_int& big_int::operator--() {
    _backend.operator--();
    return *this;
}
template<>
big_int big_int::operator++(int) {
    big_int result = *this;
    _backend.operator++();
    return result;
}
template<>
big_int big_int::operator--(int) {
    big_int result = *this;
    _backend.operator--();
    return result;
}

// math functions
template<>
big_int big_int::pow(const big_int& exponent) const {
    if (exponent <  0) return 0;
    if (exponent == 0) return 1;

    big_int base = *this;
    big_int exp = exponent;
    big_int power = base;
    while (exp > 1) {
        power = power * base;
        exp = exp - 1;
    }
    return power;
}

// properties
template<>
size_t big_int::digits() const {
    return _backend.numberOfDigits();
}

}

#endif //BIG_INT_INFINT_HPP
