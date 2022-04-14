#ifndef __UTIL_H__
#define __UTIL_H__
#include <stdio.h>
#include <assert.h>
#include <cstdint>
#define die(msg, args...) \
do {                         \
            fprintf(stderr,"(%s,%d) " msg "\n", __FUNCTION__ , __LINE__, ##args); \
            assert(0);                 \
         } while(0)

#define rdtscll(val) {                                           \
       unsigned int __a,__d;                                        \
       asm volatile("rdtsc" : "=a" (__a), "=d" (__d));              \
       (val) = ((unsigned long)__a) | (((unsigned long)__d)<<32);   \
}

uint64_t get_cpu_freq(void) ;

template <typename T>
void atomicAdd(T* ptr, T delta){
    volatile T new_val, old_val;
    do{
        old_val = *ptr;
        new_val = old_val + delta;
    }while(!__sync_bool_compare_and_swap(ptr, old_val, new_val));

}

#endif
