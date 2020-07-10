//
// Created by kirill on 10.07.2020.
//

#ifndef BIGINT__BUFF_H_
#define BIGINT__BUFF_H_

struct buff {
    size_t capacity;
    size_t ref_counter;
    uint32_t *p;
    buff(size_t c, size_t r, uint32_t *p)
        : capacity(c), ref_counter(r), p(p) {}
};

#endif //BIGINT__BUFF_H_
