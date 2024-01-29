#ifndef BIG_INT_HPP
#define BIG_INT_HPP

#include <exception>
#include <utility>

namespace atf {

class big_int_exception : std::exception {
public:
    explicit big_int_exception(std::string txt) noexcept : std::exception(), _txt(std::move(txt)) {}
    const char* what() const noexcept override {
        return _txt.c_str();
    }
private:
    std::string _txt;
};

template<typename T>
class big_int_interface;

// arithmetic operators
template<typename T>
big_int_interface<T> operator+(const big_int_interface<T>&, const big_int_interface<T>&);
template<typename T>
big_int_interface<T> operator-(const big_int_interface<T>&, const big_int_interface<T>&);
template<typename T>
big_int_interface<T> operator*(const big_int_interface<T>&, const big_int_interface<T>&);
template<typename T>
big_int_interface<T> operator/(const big_int_interface<T>&, const big_int_interface<T>&);
template<typename T>
big_int_interface<T> operator%(const big_int_interface<T>&, const big_int_interface<T>&);

// relational operators
template<typename T>
bool operator==(const big_int_interface<T>&, const big_int_interface<T>&);
template<typename T>
bool operator!=(const big_int_interface<T>&, const big_int_interface<T>&);
template<typename T>
bool operator<(const big_int_interface<T>&, const big_int_interface<T>&);
template<typename T>
bool operator<=(const big_int_interface<T>&, const big_int_interface<T>&);
template<typename T>
bool operator>(const big_int_interface<T>&, const big_int_interface<T>&);
template<typename T>
bool operator>=(const big_int_interface<T>&, const big_int_interface<T>&);

// io
template<typename T>
std::ostream& operator<<(std::ostream &s, const big_int_interface<T> &n);
template<typename T>
std::istream& operator>>(std::istream &s, big_int_interface<T> &val);

template<typename T>
class big_int_interface {
public:
    // default constructor
    big_int_interface();

    // initialization from other types
    big_int_interface(const T &v) : _backend(v) {}
    big_int_interface(int);
    big_int_interface(long);
    big_int_interface(long long);
    big_int_interface(unsigned int);
    big_int_interface(unsigned long);
    big_int_interface(unsigned long long);
    big_int_interface(const std::string&);
    big_int_interface(const char*);
    big_int_interface(const big_int_interface &min, const big_int_interface &max); // random number generation in [min, max)

    // assignment of other types
    big_int_interface& operator=(int);
    big_int_interface& operator=(long);
    big_int_interface& operator=(long long);
    big_int_interface& operator=(unsigned int);
    big_int_interface& operator=(unsigned long);
    big_int_interface& operator=(unsigned long long);
    big_int_interface& operator=(const std::string&);

    // cast to other types (throws big_int_exception when value is out of bounds)
    explicit operator int() const;
    explicit operator long() const;
    explicit operator long long() const;
    explicit operator unsigned int() const;
    explicit operator unsigned long() const;
    explicit operator unsigned long long() const;
    explicit operator std::string() const;

    // arithmetic operators
    const big_int_interface& operator++();
    const big_int_interface& operator--();
    big_int_interface operator++(int);
    big_int_interface operator--(int);
    friend big_int_interface<T> operator+<>(const big_int_interface<T>&, const big_int_interface<T>&);
    friend big_int_interface<T> operator-<>(const big_int_interface<T>&, const big_int_interface<T>&);
    friend big_int_interface<T> operator*<>(const big_int_interface<T>&, const big_int_interface<T>&);
    friend big_int_interface<T> operator/<>(const big_int_interface<T>&, const big_int_interface<T>&);
    friend big_int_interface<T> operator%<>(const big_int_interface<T>&, const big_int_interface<T>&);

    // relational operators
    friend bool operator==<>(const big_int_interface<T>&, const big_int_interface<T>&);
    friend bool operator!=<>(const big_int_interface<T>&, const big_int_interface<T>&);
    friend bool operator< <>(const big_int_interface<T>&, const big_int_interface<T>&);
    friend bool operator<=<>(const big_int_interface<T>&, const big_int_interface<T>&);
    friend bool operator><>(const big_int_interface<T>&, const big_int_interface<T>&);
    friend bool operator>=<>(const big_int_interface<T>&, const big_int_interface<T>&);

    // math functions
    big_int_interface pow(const big_int_interface&) const;

    // properties
    size_t digits() const;

    // io
    friend std::ostream& operator<<<>(std::ostream &s, const big_int_interface<T> &n);
    friend std::istream& operator>><>(std::istream &s, big_int_interface<T> &val);

    T _backend;
};

}

#include "detail/big_int_InfInt.hpp"

#endif //BIG_INT_HPP
