//
// Created by kirill on 10.07.2020.
//

#include <vector.h>
#include <cstring>
#include <cassert>

vector::vector() : size_(1u) {}

vector::vector(size_t n,
               uint32_t assign) {
    set_size(n);
    if (n <= MAX_SMALL) {
        set_small();
        std::fill(small_data, small_data + n, assign);
    } else {
        ptr = new shared_ptr_vector(std::vector<uint32_t>(n, assign));
        set_big();
    }
}

// helpful functions

void vector::set_size(size_t new_size) {
    size_ &= 1u;
    size_ |= (new_size << 1u);
}

size_t vector::get_size() const {
    return is_small() ? (size_ >> 1u) : ptr->get().size();
}

bool vector::is_small() const {
    return size_ & 1u;
}

void vector::set_small() {
    size_ |= 1u;
}

void vector::set_big() {
    size_ &= ~1u;
}

vector::vector(const vector &rhs) {
    size_ = rhs.size_;
    if (rhs.is_small()) {
        std::copy(rhs.small_data, rhs.small_data + rhs.get_size(), small_data);
    } else {
        ptr = rhs.ptr;
        ptr->inc_ref_counter();
    }
}

vector::~vector() {
    if (!is_small()) {
        ptr->dec_ref_counter();
        if (ptr->get_ref_counter() == 0) {
            delete ptr;
        }
    }
}

void vector::swap(vector &rhs) {
    std::swap(size_, rhs.size_);
    std::swap_ranges(small_data, small_data + MAX_SMALL, rhs.small_data);
}

vector &vector::operator=(const vector &rhs) {
    if (this == &rhs) {
        return *this;
    }
    vector copy(rhs);
    swap(copy);
    return *this;
}

uint32_t const &vector::operator[](size_t idx) const {
    if (is_small()) {
        return small_data[idx];
    } else {
        return ptr->get()[idx];
    }
}

uint32_t &vector::operator[](size_t idx) {
    if (is_small()) {
        return small_data[idx];
    } else {
        ptr = ptr->get_unique();
        return ptr->get()[idx];
    }
}

size_t vector::size() const {
    return get_size();
}

uint32_t vector::back() const {
    return (*this)[get_size() - 1u];
}

void vector::to_big() {
    ptr = new shared_ptr_vector(std::vector<uint32_t>(small_data, small_data + get_size()));
    set_big();
}

void vector::push_back(uint32_t const &val) {
    if (is_small() && get_size() < MAX_SMALL) {
        small_data[get_size()] = val;
        set_size(get_size() + 1);
        return;
    }
    if (is_small()) {
        to_big();
    } else {
        ptr = ptr->get_unique();
    }
    ptr->get().push_back(val);
}

void vector::pop_back() {
    if (is_small()) {
        set_size(get_size() - 1);
    } else {
        ptr = ptr->get_unique();
        ptr->get().pop_back();
    }
}

void vector::resize(size_t new_size, uint32_t assign) {
    assert(new_size >= get_size());
    if (is_small() && new_size <= MAX_SMALL) {
        std::fill(small_data + get_size(), small_data + new_size, assign);
        set_size(new_size);
    } else {
        if (is_small()) {
            to_big();
        } else {
            ptr = ptr->get_unique();
        }
        ptr->get().resize(new_size, assign);
    }
}

bool operator==(vector const &lhs, vector const &rhs) {
    if (lhs.get_size() != rhs.get_size()) {
        return false;
    }
    for (size_t i = 0; i < lhs.get_size(); i++) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}
