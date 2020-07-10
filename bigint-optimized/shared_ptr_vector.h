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
    std::vector<uint32_t> &get();
    shared_ptr_vector *get_unique();
    void inc_ref_counter();
    void dec_ref_counter();
    size_t get_ref_counter() const;
 private:
    size_t ref_counter;
    std::vector<uint32_t> data;
};

#endif //BIGINT__SHARED_PTR_VECTOR_H_
