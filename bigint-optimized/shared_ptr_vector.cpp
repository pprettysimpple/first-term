//
// Created by kirill on 10.07.2020.
//

#include "shared_ptr_vector.h"

#include <utility>

shared_ptr_vector::shared_ptr_vector(std::vector<uint32_t> rhs)
: ref_counter(1), data(std::move(rhs)) {}

shared_ptr_vector *shared_ptr_vector::get_unique() {
    if (ref_counter == 1) {
        return this;
    }
    auto *new_p = new shared_ptr_vector(data);
    ref_counter--;
    return new_p;
}
