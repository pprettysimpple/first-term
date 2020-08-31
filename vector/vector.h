//
// Created by kirill on 06.07.2020.
//

#ifndef VECTOR__VECTOR_H_
#define VECTOR__VECTOR_H_

#include <cstddef>
#include <utility>
#include <stdexcept>

template<typename T>
struct vector {
    using iterator = T*;
    using const_iterator = T const*;

    vector() = default;                     // O(1) nothrow
    vector(vector const&);                  // O(N) strong
    vector& operator=(vector const& other); // O(N) strong

    ~vector();                              // O(N) nothrow

    T& operator[](size_t i);                // O(1) nothrow
    T const& operator[](size_t i) const;    // O(1) nothrow

    T* data();                              // O(1) nothrow
    T const* data() const;                  // O(1) nothrow
    size_t size() const;                    // O(1) nothrow

    T& front();                             // O(1) nothrow
    T const& front() const;                 // O(1) nothrow

    T& back();                              // O(1) nothrow
    T const& back() const;                  // O(1) nothrow
    void push_back(T const&);               // O(1)* strong
    void pop_back();                        // O(1) nothrow

    bool empty() const;                     // O(1) nothrow

    size_t capacity() const;                // O(1) nothrow
    void reserve(size_t);                   // O(N) strong
    void shrink_to_fit();                   // O(N) strong

    void clear();                           // O(N) nothrow

    void swap(vector&);                     // O(1) nothrow

    iterator begin();                       // O(1) nothrow
    iterator end();                         // O(1) nothrow

    const_iterator begin() const;           // O(1) nothrow
    const_iterator end() const;             // O(1) nothrow

    iterator insert(const_iterator it, T const& value);         // O(N) weak

    iterator erase(const_iterator it);      // O(N) weak

    iterator erase(const_iterator first, const_iterator last);      // O(N) weak

private:
    size_t increase_capacity() const;

    void push_back_realloc(T const&);

    void new_buffer(size_t new_capacity);

    static void destroy_pref(T* pos, size_t count) noexcept;

    static void copy_construct_all(T* dest, T const* source, size_t count);

private:
    T* data_ = nullptr;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

template<typename T>
void vector<T>::destroy_pref(T* pos, size_t count) noexcept {
    while (count-- > 0) {
        (--pos)->~T();
    }
}

template<typename T>
void vector<T>::copy_construct_all(T* dest, T const* source, size_t count) {
    for (size_t i = 0; i < count; i++) {
        try {
            new(dest + i) T(source[i]);
        } catch (...) {
            destroy_pref(dest + i, i);
            throw;
        }
    }
}

template<typename T>
vector<T>::vector(vector<T> const& other) {
    new_buffer(other.size());
    try {
        copy_construct_all(data_, other.data_, other.size_);
    } catch (...) {
        operator delete(data_);
        throw;
    }
    size_ = other.size_;
    capacity_ = other.size_;
}

template<typename T>
vector<T>::~vector<T>() {
    destroy_pref(data_ + size_, size_);
    operator delete(data_);
}

template<typename T>
vector<T>& vector<T>::operator=(vector<T> const& other) {
    if (this == &other) {
        return *this;
    }
    vector<T>(other).swap(*this);
    return *this;
}

template<typename T>
void vector<T>::swap(vector<T>& other) {
    using std::swap;
    swap(data_, other.data_);
    swap(size_, other.size_);
    swap(capacity_, other.capacity_);
}

template<typename T>
T& vector<T>::operator[](size_t i) {
    return data_[i];
}

template<typename T>
T const& vector<T>::operator[](size_t i) const {
    return data_[i];
}

template<typename T>
T* vector<T>::data() {
    return data_;
}

template<typename T>
T const* vector<T>::data() const {
    return data_;
}

template<typename T>
size_t vector<T>::size() const {
    return size_;
}

template<typename T>
T& vector<T>::front() {
    return *(data_);
}

template<typename T>
T const& vector<T>::front() const {
    return *(data_);
}

template<typename T>
T& vector<T>::back() {
    return *(data_ + size_ - 1);
}

template<typename T>
T const& vector<T>::back() const {
    return *(data_ + size_ - 1);
}

template<typename T>
bool vector<T>::empty() const {
    return !size_;
}

template<typename T>
size_t vector<T>::capacity() const {
    return capacity_;
}

template<typename T>
void vector<T>::reserve(size_t new_capacity) {
    if (new_capacity > capacity_) {
        new_buffer(new_capacity);
    }
}

template<typename T>
void vector<T>::shrink_to_fit() {
    if (size_ < capacity_) {
        new_buffer(size_);
    }
}

template<typename T>
void vector<T>::clear() {
    destroy_pref(data_ + size_, size_);
    size_ = 0;
}

template<typename T>
typename vector<T>::iterator vector<T>::begin() {
    return data_;
}

template<typename T>
typename vector<T>::iterator vector<T>::end() {
    return data_ + size_;
}

template<typename T>
typename vector<T>::const_iterator vector<T>::begin() const {
    return data_;
}

template<typename T>
typename vector<T>::const_iterator vector<T>::end() const {
    return data_ + size_;
}

template<typename T>
void vector<T>::new_buffer(size_t new_capacity) {
    T* old_buff = data_;
    if (new_capacity) {
        data_ = static_cast<T*>(operator new(new_capacity * sizeof(T)));
    } else {
        data_ = nullptr;
    }
    try {
        copy_construct_all(data_, old_buff, size_);
    } catch (...) {
        operator delete(data_);
        data_ = old_buff;
        throw;
    }
    destroy_pref(old_buff + size_, size_);
    operator delete(old_buff);
    capacity_ = new_capacity;
}

template<typename T>
size_t vector<T>::increase_capacity() const {
    return 2 * capacity_ + 1;
}

template<typename T>
void vector<T>::push_back_realloc(const T& value) {
    T tmp(value);
    new_buffer(increase_capacity());
    new(data_ + size_) T(tmp);
    size_++;
}

template<typename T>
void vector<T>::pop_back() {
    data_[--size_].~T();
}

template<typename T>
void vector<T>::push_back(const T& value) {
    if (size_ == capacity_) {
        push_back_realloc(value);
    } else {
        new(data_ + size_) T(value);
        size_++;
    }
}

template<typename T>
typename vector<T>::iterator vector<T>::insert(vector<T>::const_iterator it, T const& value) {
    ptrdiff_t insert_pos = it - data_;
    ptrdiff_t current_pos = size_;
    push_back(value);
    while (current_pos > insert_pos) {
        std::swap(data_[current_pos], data_[current_pos - 1]);
        --current_pos;
    }
    return data_ + insert_pos;
}

template<typename T>
typename vector<T>::iterator vector<T>::erase(vector<T>::const_iterator it) {
    return vector<T>::erase(it, it + 1);
}

template<typename T>
typename vector<T>::iterator vector<T>::erase(vector<T>::const_iterator first, vector<T>::const_iterator last) {
    ptrdiff_t res = first - data_;
    ptrdiff_t len = last - first;
    ptrdiff_t shift = res;
    while (shift + len < size_) {
        using std::swap;
        swap(data_[shift], data_[shift + len]);
        shift++;
    }
    while (len-- > 0) {
        pop_back();
    }
    return data_ + res;
}

#endif //VECTOR__VECTOR_H_
