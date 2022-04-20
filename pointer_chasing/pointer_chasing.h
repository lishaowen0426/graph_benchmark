#pragma once
#include <algorithm>
#include "util.h"

#define buffer_malloc __malloc
#define buffer_free __free




template <size_t P>
class Entry{
public:
    void* next;
    char pad[P];
};

template<>
class Entry<0>{
public:
    void* next;
};


template<size_t P>
Entry<P>* create_random(size_t buffer_sz, size_t& len){
    typedef Entry<P> Entry_t;
    
    size_t sz = sizeof(Entry_t);
    buffer_sz = (buffer_sz + sz - 1)/sz * sz; // align buffer_sz up

    len = buffer_sz / sz;
    Entry_t* buffer = (Entry_t*)buffer_malloc(buffer_sz); 

    size_t* indices = (size_t*)malloc(len * sizeof(size_t)); 
    for(size_t i = 0; i < len; i++) indices[i] = i;

    //shuffle
#ifdef RANDOM
    std::srand(time(0));
#endif
    std::random_shuffle(indices, indices+len);


    for(size_t i = 1; i < len; i++){
        buffer[indices[i-1]].next = (void*)(&buffer[indices[i]]);
    }
    buffer[indices[len-1]].next = (void*)(&buffer[indices[0]]);

    free(indices);
    return buffer;
}

void cas_chase_fail(void* buffer, size_t len){

    void* p = buffer;
    while(len-- >0){
        p = *((void**)(p)) ;
    }

}
