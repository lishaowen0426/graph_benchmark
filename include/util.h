#ifndef __UTIL_H__
#define __UTIL_H__
#include <stdio.h>
#include <assert.h>
#include <cstdint>
#include <errno.h>

#ifdef PMEM
#include "ralloc.hpp"
#endif


#ifdef PMEM
#define __malloc RP_malloc
#define __free RP_free
#else
#define __malloc malloc
#define __free free
#endif

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

#define handle_error_en(en, msg) \
               do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define EQUAL_GRAIN(total, par) (total/par)>0? (total/par): 1

uint64_t get_cpu_freq(void) ;

#define CYCLES_TO_SEC(start, stop, freq) ((float)(stop-start))/((float)freq)

template <typename T>
void atomicAdd(T* ptr, T delta){
    volatile T new_val, old_val;
    do{
        old_val = *ptr;
        new_val = old_val + delta;
    }while(!__sync_bool_compare_and_swap(ptr, old_val, new_val));

}

inline void clflush_line(char* ptr, bool fence = false){
    if(fence)
        asm volatile ("clflush %0;mfence" : : "m" (*(volatile char*)(ptr)));
    else
        asm volatile ("clflush %0" : : "m" (*(volatile char*)(ptr)));
}




    


#endif
