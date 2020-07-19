#include "big_integer.h"

#include <cstring>
#include <stdexcept>
#include <functional>
#include <cassert>
#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>

namespace {
    big_integer const TEN(10), BASE(1000 * 1000 * 1000);

    uint32_t divide_3_2(uint32_t u3, uint32_t u2, uint32_t u1, uint32_t d2, uint32_t d1) {
        unsigned __int128 up = u3;
        up = (up << 32u) | u2;
        up = (up << 32u) | u1;
        unsigned __int128 down = d2;
        down = (down << 32u) | d1;
        return up / down;
    }

    bool is_digit(char const& c) {
        return (c >= '0' && c <= '9');
    }

    const auto AND_ = [](uint32_t a, uint32_t b) { return a & b; };
    const auto OR_ = [](uint32_t a, uint32_t b) { return a | b; };
    const auto XOR_ = [](uint32_t a, uint32_t b) { return a ^ b; };
}

big_integer::big_integer() : sign_(0), digits_(1, 0) {}

big_integer::big_integer(int a) : sign_(a < 0 ? UINT32_MAX : 0), digits_(1, (uint32_t) a) {}

big_integer::big_integer(uint32_t a) : sign_(0), digits_(1, a) {}

big_integer::big_integer(uint64_t a) : sign_(0), digits_(2) {
    digits_[0] = static_cast<uint32_t>(a);
    digits_[1] = static_cast<uint32_t>(a >> 32u);
    shrink_to_fit();
}

big_integer::big_integer(std::string const& str) : sign_(0), digits_(1, 0) {
    assert(!str.empty());
    bool result_positive = true;
    if (str[0] == '-') {
        result_positive = false;
    } else {
        assert(str[0] == '+' || is_digit(str[0]));
    }
    for (size_t pos = !is_digit(str[0]); pos < str.size(); ++pos) {
        assert(is_digit(str[pos]));
        *this = *this * TEN + (str[pos] - '0');
    }
    if (!result_positive) {
        fast_negate();
    }
}

void big_integer::shrink_to_fit() {
    while (digits_.size() > 1 && digits_.back() == sign_) {
        digits_.pop_back();
    }
}

big_integer& big_integer::operator+=(big_integer const& rhs) {
    size_t max_size = 1 + std::max(digits_.size(), rhs.digits_.size());
    digits_.resize(max_size, sign_);
    uint64_t carry = 0;
    for (size_t i = 0; i < rhs.digits_.size(); i++) {
        uint64_t new_carry = ((digits_[i] + carry + rhs.digits_[i]) >> 32u);
        digits_[i] = static_cast<uint32_t>(digits_[i] + carry + rhs.digits_[i]);
        carry = new_carry;
    }
    for (size_t i = rhs.digits_.size(); i < max_size; i++) {
        uint64_t new_carry = ((digits_[i] + carry + rhs.sign_) >> 32u);
        digits_[i] = static_cast<uint32_t>(digits_[i] + carry + rhs.sign_);
        carry = new_carry;
    }
    sign_ = (digits_.back() & (~1u)) ? UINT32_MAX : 0;
    shrink_to_fit();
    return *this;
}

big_integer& big_integer::operator-=(big_integer const& rhs) {
    size_t max_size = 1 + std::max(digits_.size(), rhs.digits_.size());
    digits_.resize(max_size, sign_);
    uint64_t carry = 1u;
    for (size_t i = 0; i < rhs.digits_.size(); i++) {
        uint64_t new_carry = ((digits_[i] + carry + ~rhs.digits_[i]) >> 32u);
        digits_[i] = static_cast<uint32_t>(digits_[i] + carry + ~rhs.digits_[i]);
        carry = new_carry;
    }
    for (size_t i = rhs.digits_.size(); i < max_size; i++) {
        uint64_t new_carry = ((digits_[i] + carry + ~rhs.sign_) >> 32u);
        digits_[i] = static_cast<uint32_t>(digits_[i] + carry + ~rhs.sign_);
        carry = new_carry;
    }
    sign_ = (digits_.back() & (~1u)) ? UINT32_MAX : 0;
    shrink_to_fit();
    return *this;
}

big_integer& big_integer::operator*=(big_integer const& rhs) {
    // TODO still many copies, if rhs >= 0 then we don't need to copy it
    bool result_positive = (rhs.sign_ == sign_);
    if (rhs.sign_ != 0) {
        *this *= -rhs;
        return fast_negate();
    }
    if (sign_ != 0) {
        fast_negate();
    }
    size_t new_sz = digits_.size() + rhs.digits_.size();
    std::vector<uint32_t> new_d(new_sz, 0);
    for (size_t i = 0; i < digits_.size(); i++) {
        uint32_t carry = 0;
        for (size_t j = 0; j < rhs.digits_.size(); ++j) {
            uint64_t cur =
                new_d[i + j]
                + static_cast<uint64_t>(digits_[i]) * rhs.digits_[j]
                + carry;
            new_d[i + j] = static_cast<uint32_t>(cur);
            carry = static_cast<uint32_t>(cur >> 32u);
        }
        new_d[i + rhs.digits_.size()] = carry;
    }
    digits_.swap(new_d);
    if (!result_positive) {
        fast_negate();
    }
    shrink_to_fit();
    return *this;
}

////////////////////////////////////////////////////////////////////////// DIV

big_integer& big_integer::divide_n_1(uint32_t rhs) {
    uint64_t carry = 0;
    for (ptrdiff_t i = (ptrdiff_t) digits_.size() - 1; i >= 0; --i) {
        uint64_t cur = digits_[i] + (carry << 32u);
        digits_[i] = static_cast<uint32_t>(cur / rhs);
        carry = static_cast<uint32_t>(cur % rhs);
    }
    shrink_to_fit();
    return *this;
}

big_integer& big_integer::subtract_power(big_integer const& rhs,
                                         size_t power) { // result = *this - rhs * 2 ^ {32 * power}
    size_t max_size = 1 + std::max(digits_.size(), rhs.digits_.size() + power);
    digits_.resize(max_size, sign_);
    uint64_t carry = 1u;
    for (size_t i = 0; i < power; i++) {
        uint64_t new_carry = ((digits_[i] + carry + UINT32_MAX) >> 32u);
        digits_[i] = static_cast<uint32_t>(digits_[i] + carry + UINT32_MAX);
        carry = new_carry;
    }
    for (size_t i = power; i < rhs.digits_.size() + power; i++) {
        uint64_t new_carry = ((digits_[i] + carry + ~rhs.digits_[i - power]) >> 32u);
        digits_[i] = static_cast<uint32_t>(digits_[i] + carry + ~rhs.digits_[i - power]);
        carry = new_carry;
    }
    for (size_t i = rhs.digits_.size() + power; i < max_size; i++) {
        uint64_t new_carry = ((digits_[i] + carry + ~rhs.sign_) >> 32u);
        digits_[i] = static_cast<uint32_t>(digits_[i] + carry + ~rhs.sign_);
        carry = new_carry;
    }
    sign_ = (digits_.back() & (~1u)) ? UINT32_MAX : 0;
    shrink_to_fit();
    return *this;
}

big_integer& big_integer::divide_m_n(big_integer const& rhs) {
    assert(digits_.size() >= 3 && rhs.digits_.size() >= 2);
    size_t n = rhs.digits_.size();
    size_t k = digits_.size() - n;
    std::vector<uint32_t> new_d(k + 1, 0);
    if (*this >= (rhs << static_cast<int>(32u * k))) {
        new_d[k] = 1;
        subtract_power(rhs, k);
    } else {
        new_d[k] = 0;
    }
    while (k > 0) {
        if (digits_.size() < rhs.digits_.size()) {
            break;
        }
        k--;
        uint32_t u3 = digits_[n + k], u2 = digits_[n + k - 1], u1 = digits_[n + k - 2];
        uint32_t d2 = rhs.digits_[n - 1], d1 = rhs.digits_[n - 2];
        if (((static_cast<uint64_t>(u3) << 32u) | u2) == ((static_cast<uint64_t>(d2) << 32u) | d1)) {
            new_d[k] = UINT32_MAX;
        } else {
            new_d[k] = divide_3_2(u3, u2, u1, d2, d1);
        }
        subtract_power(rhs * new_d[k], k);
        while (*this < 0) {
            *this += rhs;
            --new_d[k];
        }
    }
    digits_.swap(new_d);
    shrink_to_fit();
    return *this;
}

big_integer& big_integer::divide_unsigned_normalized(big_integer const& rhs) {
    if (rhs.digits_.size() == 1) {
        return divide_n_1(rhs.digits_[0]);
    }
    if (digits_.size() == 2) {
        uint64_t u = digits_[1];
        u = (u << 32u) | digits_[0];
        uint64_t v = rhs.digits_[0];
        if (rhs.digits_.size() == 2) {
            v |= static_cast<uint64_t>(rhs.digits_[1]) << 32u;
        }
        return *this = u / v;
    }
    return divide_m_n(rhs);
}

big_integer& big_integer::divide_unsigned(big_integer& rhs) {
    if (digits_.size() < rhs.digits_.size()) {
        return *this = 0;
    }
    int clz = __builtin_clz(rhs.digits_.back());
    if (clz) {
        *this <<= clz;
        return divide_unsigned_normalized(rhs <<= clz);
    } else {
        return divide_unsigned_normalized(rhs);
    }
}

big_integer& big_integer::operator/=(big_integer rhs) {
    if (rhs == 0) {
        throw std::overflow_error("Divide by zero exception");;
    }
    bool result_positive = (rhs.sign_ == sign_);
    if (rhs.sign_ != 0) {
        rhs.fast_negate();
    }
    if (sign_ != 0) {
        fast_negate();
    }
    big_integer copy_rhs(rhs);
    *this = divide_unsigned(copy_rhs);
    if (!result_positive) {
        fast_negate();
    }
    return *this;
}

big_integer& big_integer::operator%=(big_integer const& rhs) {
    return *this -= (*this / rhs) * rhs;
}

////////////////////////////////////////////////////////////////////////// DIV_END

void big_integer::bit_operation(big_integer const& rhs, std::function<uint32_t(uint32_t, uint32_t)> const& f) {
    if (digits_.size() < rhs.digits_.size()) {
        digits_.resize(rhs.digits_.size(), sign_);
    }
    for (size_t i = 0; i < digits_.size(); ++i) {
        uint32_t cur = i < rhs.digits_.size() ? rhs.digits_[i] : rhs.sign_;
        digits_[i] = f(digits_[i], cur);
    }
    sign_ = f(sign_, rhs.sign_);
}

big_integer& big_integer::operator&=(big_integer const& rhs) {
    bit_operation(rhs, AND_);
    return *this;
}

big_integer& big_integer::operator|=(big_integer const& rhs) {
    bit_operation(rhs, OR_);
    return *this;
}

big_integer& big_integer::operator^=(big_integer const& rhs) {
    bit_operation(rhs, XOR_);
    return *this;
}

big_integer& big_integer::operator<<=(int rhs) {
    assert(rhs >= 0);
    if (rhs == 0) {
        return *this;
    }
    digits_.resize(
        digits_.size() + rhs / (32u) + 1,
        sign_);
    if (rhs / (32u) != 0) {
        for (size_t i = digits_.size(); i > 0; --i) {
            ptrdiff_t pos_from = static_cast<ptrdiff_t>(i - 1) - rhs / (32u);
            digits_[i - 1] = (pos_from >= 0 ? digits_[pos_from] : 0);
        }
    }
    if (rhs % (32u) != 0) {
        for (size_t i = digits_.size(); i > 0; --i) {
            uint32_t next = (i < 2 ? 0 : digits_[i - 2]);
            uint32_t cur = digits_[i - 1];
            digits_[i - 1] =
                static_cast<uint32_t>((((
                    static_cast<uint64_t>(cur) << 32u) | next)
                    <<
                    static_cast<uint64_t>(rhs % 32u))
                    >> 32u);
        }
    }
    shrink_to_fit();
    return *this;
}

big_integer& big_integer::operator>>=(int rhs) {
    assert(rhs >= 0);
    if (rhs == 0) {
        return *this;
    }
    if (rhs / (32u) != 0) {
        for (size_t i = 0; i < digits_.size(); i++) {
            size_t pos_from = i + rhs / (32u);
            digits_[i] = (pos_from < digits_.size() ? digits_[pos_from] : sign_);
        }
    }
    uint32_t prev = sign_;
    if (rhs % (32u) != 0) {
        for (size_t i = digits_.size(); i > 0; --i) {
            uint32_t cur = digits_[i - 1];
            digits_[i - 1] = static_cast<uint32_t>((
                (static_cast<uint64_t>(prev) << 32u) | cur)
                >>
                static_cast<uint32_t>(rhs % (32u)));
            prev = cur;
        }
    }
    shrink_to_fit();
    return *this;
}

big_integer big_integer::operator+() const {
    return *this;
}

big_integer big_integer::operator-() const {
    return ++(~(*this));
}

big_integer big_integer::operator~() const {
    return big_integer(*this).bit_not();
}

big_integer& big_integer::add_one() {
    size_t pos = 0;
    while (pos < digits_.size() && digits_[pos] == UINT32_MAX) {
        digits_[pos++] = 0;
    }
    if (pos == digits_.size()) {
        digits_.push_back(sign_ + 1);
        sign_ = (digits_.back() & (~1u)) ? UINT32_MAX : 0;
    } else {
        digits_[pos]++;
    }
    shrink_to_fit();
    return *this;
}

big_integer& big_integer::operator++() {
    add_one();
    return *this;
}

big_integer big_integer::operator++(int) {
    big_integer r = *this;
    ++*this;
    return r;
}

big_integer& big_integer::operator--() {
    return *this -= 1;
}

big_integer big_integer::operator--(int) {
    big_integer r = *this;
    --*this;
    return r;
}

big_integer operator+(big_integer a, big_integer const& b) {
    return a += b;
}

big_integer operator-(big_integer a, big_integer const& b) {
    return a -= b;
}

big_integer operator*(big_integer a, big_integer const& b) {
    return a *= b;
}

big_integer operator/(big_integer a, big_integer const& b) {
    return a /= b;
}

big_integer operator%(big_integer a, big_integer const& b) {
    return a %= b;
}

big_integer operator&(big_integer a, big_integer const& b) {
    return a &= b;
}

big_integer operator|(big_integer a, big_integer const& b) {
    return a |= b;
}

big_integer operator^(big_integer a, big_integer const& b) {
    return a ^= b;
}

big_integer operator<<(big_integer a, int b) {
    return a <<= b;
}

big_integer operator>>(big_integer a, int b) {
    return a >>= b;
}

bool operator==(big_integer const& a, big_integer const& b) {
    return a.sign_ == b.sign_ && a.digits_ == b.digits_;
}

bool operator!=(big_integer const& a, big_integer const& b) {
    return !(a == b);
}

bool operator<(big_integer const& a, big_integer const& b) {
    if (a.sign_ ^ b.sign_) return a.sign_;
    if (a.digits_.size() != b.digits_.size()) return (a.digits_.size() < b.digits_.size());
    for (ptrdiff_t i = a.digits_.size() - 1; i >= 0; --i) {
        if (a.digits_[i] != b.digits_[i]) {
            return (a.digits_[i] < b.digits_[i]);
        }
    }
    return false;
}

bool operator>(big_integer const& a, big_integer const& b) {
    return b < a;
}

bool operator<=(big_integer const& a, big_integer const& b) {
    return !(a > b);
}

bool operator>=(big_integer const& a, big_integer const& b) {
    return !(a < b);
}

std::string to_string(big_integer const& rhs) {
    if (rhs == 0) {
        return "0";
    }
    big_integer copy(rhs);
    bool result_positive = true;
    if (copy.sign_) {
        result_positive = false;
        copy.fast_negate();
    }
    std::string ans;
    while (copy != 0) {
        std::stringstream cur;
        std::string tmp;
        cur << std::setfill('0') << std::setw(9) << std::to_string((copy % BASE).digits_[0]);
        cur >> tmp;
        std::reverse(tmp.begin(), tmp.end());
        ans += tmp;
        copy /= BASE;
    }
    while (ans.size() > 1 && ans.back() == '0') {
        ans.pop_back();
    }
    if (!result_positive) {
        ans.push_back('-');
    }
    std::reverse(ans.begin(), ans.end());
    return ans;
}

big_integer& big_integer::bit_not() {
    sign_ = ~sign_;
    for (uint32_t& cur : digits_) {
        cur = ~cur;
    }
    shrink_to_fit();
    return *this;
}
big_integer& big_integer::fast_negate() {
    bit_not();
    return add_one();
}

std::ostream& operator<<(std::ostream& s, big_integer const& a) {
    return s << to_string(a);
}
