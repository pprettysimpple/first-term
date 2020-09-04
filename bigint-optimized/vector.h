//
// Created by kirill on 09.07.2020.
//

#ifndef BIGINT__VECTOR_H_
#define BIGINT__VECTOR_H_
#include <memory>
#include <shared_ptr_vector.h>

struct vector {
    vector();
    explicit vector(size_t n);
    vector(size_t n, uint32_t assign);
    explicit vector(vector const &rhs);
    vector &operator=(vector const &rhs);
    ~vector();

    uint32_t const &operator[](size_t idx) const;
    uint32_t &operator[](size_t idx);
    size_t size() const;
    uint32_t back() const;
    void push_back(uint32_t const &);
    void pop_back();
    void resize(size_t new_size, uint32_t assign);
    void swap(vector &rhs);
    friend bool operator==(vector const &, vector const &);

 private:
    static constexpr size_t MAX_SMALL = 8;

 private:
    void set_size(size_t new_size);
    size_t get_size() const;
    bool is_small() const;
    void set_small();
    void set_big();
    void to_big();

 private:
    union {
        shared_ptr_vector *ptr;
        uint32_t small_data[MAX_SMALL];                             // this part of union is always bigger than other one
    };
    size_t size_;
}; // 32 bytes

#endif //BIGINT__VECTOR_H_
