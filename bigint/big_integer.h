#ifndef BIG_INTEGER_H
#define BIG_INTEGER_H

#include <cstddef>
#include <gmp.h>
#include <iosfwd>
#include <cstdint>
#include <vector>
#include <functional>

struct big_integer {
    big_integer();
    big_integer(big_integer const &other) = default;
    big_integer(int a);
    big_integer(uint32_t a);
    big_integer(uint64_t a);
    explicit big_integer(std::string const &str);
    ~big_integer() = default;

    big_integer &operator=(big_integer const &other) = default;

    big_integer &operator+=(big_integer const &rhs);
    big_integer &operator-=(big_integer const &rhs);
    big_integer &operator*=(big_integer const &rhs);
    big_integer &operator/=(big_integer const &rhs);
    big_integer &operator%=(big_integer const &rhs);

    big_integer &operator&=(big_integer const &rhs);
    big_integer &operator|=(big_integer const &rhs);
    big_integer &operator^=(big_integer const &rhs);

    big_integer &operator<<=(int rhs);
    big_integer &operator>>=(int rhs);

    big_integer operator+() const;
    big_integer operator-() const;
    big_integer operator~() const;

    big_integer &operator++();
    big_integer operator++(int);

    big_integer &operator--();
    big_integer operator--(int);

    friend bool operator==(big_integer const &a, big_integer const &b);
    friend bool operator!=(big_integer const &a, big_integer const &b);
    friend bool operator<(big_integer const &a, big_integer const &b);
    friend bool operator>(big_integer const &a, big_integer const &b);
    friend bool operator<=(big_integer const &a, big_integer const &b);
    friend bool operator>=(big_integer const &a, big_integer const &b);

    friend std::string to_string(big_integer const &a);
    uint32_t cast_to_uint32();

 private:
    void _shrink_to_fit();
    void _bit(big_integer const &rhs, std::function<uint32_t(uint32_t, uint32_t)> const &f);
    big_integer &_divide_unsigned(big_integer &rhs);
    big_integer &_divide_unsigned_normalized(big_integer &rhs);
    big_integer &_divide_m_n(big_integer &rhs);
    big_integer &_divide_n_1(uint32_t);
    friend uint32_t divide_3_2(uint32_t u3, uint32_t u2, uint32_t u1, uint32_t d2, uint32_t d1);
    void add_one();

 private:
    uint32_t sign_;
    std::vector<uint32_t> digits_;

};

big_integer operator+(big_integer a, big_integer const &b);
big_integer operator-(big_integer a, big_integer const &b);
big_integer operator*(big_integer a, big_integer const &b);
big_integer operator/(big_integer a, big_integer const &b);
big_integer operator%(big_integer a, big_integer const &b);

big_integer operator&(big_integer a, big_integer const &b);
big_integer operator|(big_integer a, big_integer const &b);
big_integer operator^(big_integer a, big_integer const &b);

big_integer operator<<(big_integer a, int b);
big_integer operator>>(big_integer a, int b);

bool operator==(big_integer const &a, big_integer const &b);
bool operator!=(big_integer const &a, big_integer const &b);
bool operator<(big_integer const &a, big_integer const &b);
bool operator>(big_integer const &a, big_integer const &b);
bool operator<=(big_integer const &a, big_integer const &b);
bool operator>=(big_integer const &a, big_integer const &b);

std::string to_string(big_integer const &a);
std::ostream &operator<<(std::ostream &s, big_integer const &a);

#endif // BIG_INTEGER_H
