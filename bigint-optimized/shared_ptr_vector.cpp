//
// Created by kirill on 10.07.2020.
//

#include "shared_ptr_vector.h"

#include <utility>

shared_ptr_vector::shared_ptr_vector(std::vector<uint32_t> rhs)
: ref_counter(1), data(std::move(rhs)) {}

std::vector<uint32_t> &shared_ptr_vector::get() {
    return data;
}

shared_ptr_vector *shared_ptr_vector::get_unique() {
    if (ref_counter == 1) {
        return this;
    }
    auto *new_p = new shared_ptr_vector(data);
    dec_ref_counter();
    return new_p;
}

void shared_ptr_vector::inc_ref_counter() {
    ++ref_counter;
}

void shared_ptr_vector::dec_ref_counter() {
    --ref_counter;
}

size_t shared_ptr_vector::get_ref_counter() const {
    return ref_counter;
}



