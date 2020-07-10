#include "big_integer.h"

#include <cstring>
#include <stdexcept>
#include <functional>
#include <cassert>
#include <algorithm>
#include <string>

namespace {
    static const big_integer TEN(10);

    uint32_t divide_3_2(
        uint32_t u3, uint32_t u2, uint32_t u1,
        uint32_t d2, uint32_t d1) {
        unsigned __int128 up = u3;
        up = (up << 32u) | u2;
        up = (up << 32u) | u1;
        unsigned __int128 down = d2;
        down = (down << 32u) | d1;
        return up / down;
    }
}

big_integer::big_integer() : sign_(0), digits_(1, 0) {}

big_integer::big_integer(int a) : sign_(a < 0 ? UINT32_MAX : 0), digits_(1, static_cast<uint32_t>(a)) {}

big_integer::big_integer(uint32_t a) : sign_(0), digits_(1, a) {}

big_integer::big_integer(uint64_t a) : sign_(0), digits_(2, 0) {
    digits_[0] = static_cast<uint32_t>(a);
    digits_[1] = static_cast<uint32_t>(a >> 32u);
}

big_integer::big_integer(std::string const &str) : sign_(false), digits_(1, 0) {
    if (str.empty() || str == "0" || str == "-0" || str == "+0") {
        return;
    }
    for (size_t pos = (str[0] > '9' || str[0] < '0'); pos < str.size(); ++pos) {
        *this = *this * TEN + (str[pos] - '0');
    }
    if (str[0] == '-') {
        *this = -(*this);
    }
}

void big_integer::_shrink_to_fit() {
    while (digits_.size() > 1 && digits_.back() == sign_) {
        digits_.pop_back();
    }
}

big_integer &big_integer::operator+=(big_integer const &rhs) {
    size_t max_size = 1 + std::max(digits_.size(), rhs.digits_.size());
    digits_.resize(max_size, sign_);
    uint32_t carry = 0;
    for (size_t i = 0; i < max_size; i++) {
        carry = __builtin_uadd_overflow(
            digits_[i],
            carry,
            &(digits_[i]));
        carry += __builtin_uadd_overflow(
            digits_[i],
            (i < rhs.digits_.size() ? rhs.digits_[i] : rhs.sign_),
            &(digits_[i]));
    }
    sign_ = (digits_.back() & (~1u)) ? UINT32_MAX : 0;
    _shrink_to_fit();
    return *this;
}

//big_integer &big_integer::operator-=(big_integer const &rhs) {
//    return *this += (-rhs);
//}

big_integer &big_integer::operator-=(big_integer const &rhs) {
    size_t max_size = 1 + std::max(digits_.size(), rhs.digits_.size());
    digits_.resize(max_size, sign_);
    uint32_t carry = 1u;
    for (size_t i = 0; i < max_size; i++) {
        carry = __builtin_uadd_overflow(
            digits_[i],
            carry,
            &(digits_[i]));
        carry += __builtin_uadd_overflow(
            digits_[i],
            (i < rhs.digits_.size() ? ~rhs.digits_[i] : ~rhs.sign_),
            &(digits_[i]));
    }
    sign_ = (digits_.back() & (~1u)) ? UINT32_MAX : 0;
    _shrink_to_fit();
    return *this;
}

big_integer &big_integer::operator*=(big_integer const &rhs) {
    if (sign_) return *this = -((-*this) * rhs);
    if (rhs.sign_) return *this = -(*this *= (-rhs));

    size_t new_sz = 1 + digits_.size() + rhs.digits_.size();
    vector new_d(new_sz, 0);
    uint32_t carry = 0;
    for (size_t i = 0; i < digits_.size(); i++) {
        for (size_t j = 0; j < rhs.digits_.size() || carry; ++j) {
            uint64_t cur =
                new_d[i + j] +
                        static_cast<uint64_t>(digits_[i])
                        * (j < rhs.digits_.size() ? rhs.digits_[j] : 0)
                    + carry;
            new_d[i + j] = static_cast<uint32_t>(cur);
            carry = static_cast<uint32_t>(cur >> 32u);
        }
    }
    digits_.swap(new_d);
    _shrink_to_fit();
    return *this;
}

////////////////////////////////////////////////////////////////////////// DIV

void big_integer::_divide_n_1(uint32_t rhs) {
    uint64_t carry = 0;
    for (int i = (int) digits_.size() - 1; i >= 0; --i) {
        uint64_t cur = digits_[i] + (carry << 32u);
        digits_[i] = static_cast<uint32_t>(cur / rhs);
        carry = static_cast<uint32_t>(cur % rhs);
    }
    _shrink_to_fit();
}

void big_integer::_divide_m_n(big_integer const &rhs) {
    assert(digits_.size() >= 3 && rhs.digits_.size() >= 2);
    big_integer copy_rhs(rhs);
    size_t n = rhs.digits_.size();
    size_t k = digits_.size() - n;
    vector new_d(k + 1, 0);
    copy_rhs <<= static_cast<int>(32u * k);
    if (*this >= copy_rhs) {
        new_d[k] = 1;
        *this -= copy_rhs;
    } else {
        new_d[k] = 0;
    }
    while (k > 0) {
        if (digits_.size() < copy_rhs.digits_.size()) {
            break;
        }
        k--;
        copy_rhs >>= 32u;
        uint32_t u3 = digits_[n + k], u2 = digits_[n + k - 1], u1 = digits_[n + k - 2];
        uint32_t d2 = copy_rhs.digits_[n + k - 1], d1 = copy_rhs.digits_[n + k - 2];
        if (((static_cast<uint64_t>(u3) << 32u) | u2) == ((static_cast<uint64_t>(d2) << 32u) | d1)) {
            new_d[k] = UINT32_MAX;
        } else {
            new_d[k] = divide_3_2(u3, u2, u1, d2, d1);
        }
        *this -= copy_rhs * new_d[k];
        if (*this < 0) {
            *this += rhs;
            --new_d[k];
        }
    }
    sign_ = 0u;
    digits_.swap(new_d);
    _shrink_to_fit();
}

void big_integer::_divide_unsigned_normalized(big_integer const &rhs) {
    if (rhs.digits_.size() == 1) {
        _divide_n_1(rhs.digits_[0]);
        return;
    }
    if (digits_.size() == 2) {
        uint64_t u = digits_[1];
        u = (u << 32u) | digits_[0];
        uint64_t v = rhs.digits_[0];
        if (rhs.digits_.size() == 2) {
            v |= static_cast<uint64_t>(rhs.digits_[1]) << 32u;
        }
        *this = u / v;
        return;
    }
    _divide_m_n(rhs);
}

void big_integer::_divide_unsigned(big_integer rhs) {
    if (digits_.size() < rhs.digits_.size()) {
        *this = 0;
        return;
    }
    int clz = __builtin_clz(rhs.digits_.back());
    if (clz) {
        *this <<= clz;
        _divide_unsigned_normalized(rhs <<= clz);
        return;
    } else {
        _divide_unsigned_normalized(rhs);
        return;
    }
}

big_integer &big_integer::operator/=(big_integer const &rhs) {
    if (rhs == 0) {
        throw std::overflow_error("Divide by zero exception");
    }
    if (sign_) return *this = -((-*this) / rhs);
    if (rhs.sign_) return *this = -(*this /= (-rhs));
    _divide_unsigned(rhs);
    return *this;
}

big_integer &big_integer::operator%=(big_integer const &rhs) {
    return *this -= (*this / rhs) * rhs;
}

////////////////////////////////////////////////////////////////////////// DIV_END

static const auto AND_ = [](uint32_t a, uint32_t b) { return a & b; };
static const auto OR_ = [](uint32_t a, uint32_t b) { return a | b; };
static const auto XOR_ = [](uint32_t a, uint32_t b) { return a ^ b; };

void big_integer::_bit(big_integer const &rhs, std::function<uint32_t(uint32_t, uint32_t)> const &f) {
    if (digits_.size() < rhs.digits_.size()) {
        digits_.resize(rhs.digits_.size(), sign_);
    }
    for (size_t i = 0; i < digits_.size(); ++i) {
        uint32_t cur = i < rhs.digits_.size() ? rhs.digits_[i] : rhs.sign_;
        digits_[i] = f(digits_[i], cur);
    }
    sign_ = f(sign_, rhs.sign_);
}

big_integer &big_integer::operator&=(big_integer const &rhs) {
    this->_bit(rhs, AND_);
    return *this;
}

big_integer &big_integer::operator|=(big_integer const &rhs) {
    this->_bit(rhs, OR_);
    return *this;
}

big_integer &big_integer::operator^=(big_integer const &rhs) {
    this->_bit(rhs, XOR_);
    return *this;
}

big_integer &big_integer::operator<<=(int rhs) {
    if (!rhs) return *this;
    if (rhs < 0) {
        return (*this) >>= (-rhs);
    }
    digits_.resize(
        digits_.size() + rhs / (32u) + 2,
        sign_);
    if (rhs / (32u) != 0) {
        for (ptrdiff_t i = digits_.size() - 1; i >= 0; --i) {
            ptrdiff_t pos_from = static_cast<ptrdiff_t>(i) - rhs / (32u);
            digits_[i] = (pos_from >= 0 ? digits_[pos_from] : 0);
        }
    }
    if (rhs % (32u) != 0) {
        for (ptrdiff_t i = digits_.size() - 1; i >= 0; --i) {
            uint32_t next = (i == 0 ? 0 : digits_[i - 1]);
            uint32_t cur = digits_[i];
            digits_[i] =
                static_cast<uint32_t>((((
                    static_cast<uint64_t>(cur) << 32u) | next)
                    <<
                    static_cast<uint64_t>(rhs % 32u))
                    >> 32u);
        }
    }
    _shrink_to_fit();
    return *this;
}

big_integer &big_integer::operator>>=(int rhs) {
    if (!rhs) return *this;
    if (rhs < 0) {
        return (*this) <<= (-rhs);
    }
    if (rhs / (32u) != 0) {
        for (size_t i = 0; i < digits_.size(); i++) {
            size_t pos_from = i + rhs / (32u);
            digits_[i] = (pos_from < digits_.size() ? digits_[pos_from] : sign_);
        }
    }
    uint32_t prev = sign_;
    if (rhs % (32u) != 0) {
        for (int i = (int) digits_.size() - 1; i >= 0; --i) {
            uint32_t cur = digits_[i];
            digits_[i] = static_cast<uint32_t>((
                (static_cast<uint64_t>(prev) << 32u) | cur)
                >>
                static_cast<uint32_t>(rhs % (32u)));
            prev = cur;
        }
    }
    _shrink_to_fit();
    return *this;
}

big_integer big_integer::operator+() const {
    return *this;
}

big_integer big_integer::operator-() const {
    return ++(~(*this));
}

big_integer big_integer::operator~() const {
    big_integer ret;
    ret.digits_ = digits_;
    ret.sign_ = ~sign_;
    for (size_t i = 0; i < ret.digits_.size(); i++) {
        ret.digits_[i] = ~ret.digits_[i];
    }
    ret._shrink_to_fit();
    return ret;
}

void big_integer::add_one() {
    size_t pos = 0;
    while (pos < digits_.size() && digits_[pos] == UINT32_MAX) {
        digits_[pos++] = 0;
    }
    if (pos == digits_.size()) {
        digits_.push_back(sign_ + 1u);
        sign_ = (digits_.back() & (~1u)) ? UINT32_MAX : 0;
    } else {
        digits_[pos]++;
    }
    _shrink_to_fit();
}

big_integer &big_integer::operator++() {
    add_one();
    return *this;
}

big_integer big_integer::operator++(int) {
    big_integer r = *this;
    ++*this;
    return r;
}

big_integer &big_integer::operator--() {
    return *this -= 1;
}

big_integer big_integer::operator--(int) {
    big_integer r = *this;
    --*this;
    return r;
}

big_integer operator+(big_integer a, big_integer const &b) {
    return a += b;
}

big_integer operator-(big_integer a, big_integer const &b) {
    return a -= b;
}

big_integer operator*(big_integer a, big_integer const &b) {
    return a *= b;
}

big_integer operator/(big_integer a, big_integer const &b) {
    return a /= b;
}

big_integer operator%(big_integer a, big_integer const &b) {
    return a %= b;
}

big_integer operator&(big_integer a, big_integer const &b) {
    return a &= b;
}

big_integer operator|(big_integer a, big_integer const &b) {
    return a |= b;
}

big_integer operator^(big_integer a, big_integer const &b) {
    return a ^= b;
}

big_integer operator<<(big_integer a, int b) {
    return a <<= b;
}

big_integer operator>>(big_integer a, int b) {
    return a >>= b;
}

bool operator==(big_integer const &a, big_integer const &b) {
    return a.sign_ == b.sign_ && a.digits_ == b.digits_;
}

bool operator!=(big_integer const &a, big_integer const &b) {
    return !(a == b);
}

bool operator<(big_integer const &a, big_integer const &b) {
    if (a.sign_ ^ b.sign_) return a.sign_;
    if (a.sign_) return (-b < -a);
    if (a.digits_.size() != b.digits_.size()) return a.digits_.size() < b.digits_.size();
    for (ptrdiff_t i = a.digits_.size() - 1; i >= 0; --i)
        if (a.digits_[i] != b.digits_[i])
            return (a.digits_[i] < b.digits_[i]);
    return false;
}

bool operator>(big_integer const &a, big_integer const &b) {
    return b < a;
}

bool operator<=(big_integer const &a, big_integer const &b) {
    return !(a > b);
}

bool operator>=(big_integer const &a, big_integer const &b) {
    return !(a < b);
}

std::string to_string(big_integer const &a) {
    if (a == 0) {
        return "0";
    }
    if (a.sign_) {
        return "-" + to_string(-a);
    }
    big_integer copy_a(a);
    std::string ans;
    while (copy_a != 0) {
        ans += std::to_string((copy_a % 10).cast_to_uint32());
        copy_a /= 10;
    }
    std::reverse(ans.begin(), ans.end());
    return ans;
}

uint32_t big_integer::cast_to_uint32() {
    return digits_[0];
}

std::ostream &operator<<(std::ostream &s, big_integer const &a) {
    return s << to_string(a);
}
