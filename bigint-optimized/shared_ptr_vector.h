//
// Created by kirill on 10.07.2020.
//

#ifndef BIGINT__SHARED_PTR_VECTOR_H_
#define BIGINT__SHARED_PTR_VECTOR_H_

#include <vector>
#include <cstdint>
#include <cstddef>

struct shared_ptr_vector {
    explicit shared_ptr_vector(std::vector<uint32_t> rhs);
    shared_ptr_vector *get_unique();
    size_t ref_counter;
    std::vector<uint32_t> data;
};

#endif //BIGINT__SHARED_PTR_VECTOR_H_
