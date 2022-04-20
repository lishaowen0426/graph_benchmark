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
Entry<P>* create_random(size_t& buffer_sz, size_t& len, bool shuffle){
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
    if(shuffle){
        printf("shuffle!\n");
        std::random_shuffle(indices, indices+len);
    }

    for(size_t i = 1; i < len; i++){
        buffer[indices[i-1]].next = (void*)(&buffer[indices[i]]);
    }
    buffer[indices[len-1]].next = (void*)(&buffer[indices[0]]);

    free(indices);
    return buffer;
}

void read_chase(void* buffer, size_t count){

    void* p = buffer;
    while(count-- >0){
        p = *((void**)(p)) ;
    }

}


void cas_chase_fail(void* buffer, size_t count){
   
    void* p = buffer;
    void* oldV = (void*)0x0;
    void* newV = (void*)0x1;
    while(count-- > 0){
        p = __sync_val_compare_and_swap((void**)p, oldV, newV);
    }
}


void cas_chase_success(void* buffer, size_t len, size_t count, size_t sz ){
   
}


void cas_hub(void* buffer,size_t len,  size_t count, bool success, size_t sz){
    if(success){
        cas_chase_success(buffer,len,count,sz);
    }else{
        cas_chase_fail(buffer,count);
    }

}
