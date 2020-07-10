//
// Created by kirill on 10.07.2020.
//

#include <vector.h>
#include <cstring>

vector::vector() : size_small_data(1u) {}

vector::vector(size_t n,
               uint32_t assign) {
    set_size(n);
    if (n <= MAX_SMALL) {
        set_small();
        std::fill(small_data, small_data + n, assign);
    } else {
        set_big();
        auto *new_p = static_cast<uint32_t*>(operator new [] (n * 4));
        try {
            buffer = new buff(n, 1, new_p);
        } catch (...) {
            operator delete[] (new_p);
        }
        std::fill(buffer->p, buffer->p + n, assign);
    }
}

// helpful functions

void vector::set_size(size_t new_size) {
    size_small_data &= 1u;
    size_small_data |= (new_size << 1u);
}

size_t vector::get_size() const {
    return (size_small_data >> 1u);
}

bool vector::is_small() const {
    return size_small_data & 1u;
}

void vector::set_small() {
    size_small_data |= 1u;
}

void vector::set_big() {
    size_small_data &= ~1u;
}

vector::vector(const vector &rhs) {
    size_small_data = rhs.size_small_data;
    if (rhs.is_small()) {
        std::copy(rhs.small_data, rhs.small_data + rhs.get_size(), small_data);
    } else {
        buffer = rhs.buffer;
        ++(buffer->ref_counter);
    }
}

vector::~vector() {
    if (!is_small()) {
        --(buffer->ref_counter);
        if (buffer->ref_counter == 0) {
            operator delete [] (buffer->p);
            delete buffer;
        }
    }
}

void vector::swap(vector &rhs) {
    std::swap(size_small_data, rhs.size_small_data);
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
        return buffer->p[idx];
    }
}

void vector::make_unique() {
    if (buffer->ref_counter == 1) {
        return;
    }
    auto *new_p = static_cast<uint32_t *>(operator new[](4 * get_size()));
    std::copy(buffer->p, buffer->p + get_size(), new_p);
    --(buffer->ref_counter);
    try {
        buffer = new buff(get_size(), 1, new_p);
    } catch (...) {
        operator delete[] (new_p);
        ++(buffer->ref_counter);
    }
}

uint32_t &vector::operator[](size_t idx) {
    if (is_small()) {
        return small_data[idx];
    } else {
        make_unique();
        return buffer->p[idx];
    }
}

size_t vector::size() const {
    return get_size();
}

uint32_t vector::back() const {
    return (*this)[get_size() - 1u];
}

void vector::to_big() {
    auto *new_p = static_cast<uint32_t*>(operator new [] (2 * MAX_SMALL * 4));
    std::copy(small_data, small_data + get_size(), new_p);
    try {
        buffer = new buff(2 * MAX_SMALL, 1, new_p);
    } catch (...) {
        operator delete[] (new_p);
    }
    set_big();
}

size_t vector::increase_capacity() const {
    return 2 * buffer->capacity + 1;
}

// buffer should be unique
void vector::new_buffer(size_t new_capacity) {
    auto *new_p = static_cast<uint32_t*>(operator new [] (new_capacity * 4));
    std::copy(buffer->p, buffer->p + get_size(), new_p);
    operator delete[] (buffer->p);
    buffer->p = new_p;
    buffer->capacity = new_capacity;
}

void vector::push_back_realloc(uint32_t const &val) {
    new_buffer(increase_capacity());
    buffer->p[get_size() - 1] = val;
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
        make_unique();
    }
    if (get_size() == buffer->capacity) {
        push_back_realloc(val);
    } else {
        buffer->p[get_size()] = val;
        set_size(get_size() + 1);
    }
}

void vector::pop_back() {
    set_size(get_size() - 1);
}

void vector::resize(size_t new_size, uint32_t assign) {
    if (is_small() && new_size <= MAX_SMALL) {
        std::fill(small_data + get_size(), small_data + new_size, assign);
    } else {
        if (is_small()) {
            to_big();
        } else {
            make_unique();
        }
        new_buffer(new_size);
        std::fill(buffer->p + get_size(), buffer->p + new_size, assign);
    }
    set_size(new_size);
}
bool operator==(const vector &lhs, const vector &rhs) {
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
